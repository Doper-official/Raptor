import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;

import java.lang.invoke.MethodHandle;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class Raptor implements AutoCloseable {

    private static final Linker LINKER = Linker.nativeLinker();

    private final Arena libraryArena = Arena.ofShared();
    private final SymbolLookup library;
    private final MethodHandle nativeCreate;
    private final MethodHandle nativeRun;
    private final MethodHandle nativeDestroy;
    private final MethodHandle nativeLastError;

    private MemorySegment app = MemorySegment.NULL;
    private boolean closed;

    public Raptor() throws Exception {
        Path dllPath = findDll();

        library = SymbolLookup.libraryLookup(dllPath, libraryArena);

        nativeCreate = bind(
                "raptor_first_app_create",
                FunctionDescriptor.of(ValueLayout.ADDRESS)
        );

        nativeRun = bind(
                "raptor_first_app_run",
                FunctionDescriptor.of(
                        ValueLayout.JAVA_INT,
                        ValueLayout.ADDRESS
                )
        );

        nativeDestroy = bind(
                "raptor_first_app_destroy",
                FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        nativeLastError = bind(
                "raptor_last_error",
                FunctionDescriptor.of(ValueLayout.ADDRESS)
        );
    }

    private MethodHandle bind(
            String name,
            FunctionDescriptor descriptor
    ) {
        MemorySegment address = library.find(name)
                .orElseThrow(() ->
                        new UnsatisfiedLinkError(
                                "Missing native symbol: " + name
                        )
                );

        return LINKER.downcallHandle(address, descriptor);
    }

    private static Path findDll() {
        Path[] candidates = {
                // Running from Engine/
                Path.of("Test builds", "raptor.dll"),
                Path.of("Release builds", "raptor.dll"),

                // Running from Engine/java/
                Path.of("..", "Test builds", "raptor.dll"),
                Path.of("..", "Release builds", "raptor.dll")
        };

        for (Path path : candidates) {
            if (Files.isRegularFile(path)) {
                return path.toAbsolutePath().normalize();
            }
        }

        throw new IllegalStateException(
                "Could not find raptor.dll. Working directory: "
                        + Path.of("").toAbsolutePath()
        );
    }

    private String getLastError() throws Throwable {
        MemorySegment error = (MemorySegment) nativeLastError.invokeExact();

        if (error.equals(MemorySegment.NULL)) {
            return "Unknown native error";
        }

        return error.reinterpret(4096)
                .getString(0, StandardCharsets.UTF_8);
    }

    public void create() throws Throwable {
        if (closed) {
            throw new IllegalStateException("Raptor is closed");
        }

        if (!app.equals(MemorySegment.NULL)) {
            throw new IllegalStateException(
                    "Raptor application already created"
            );
        }

        app = (MemorySegment) nativeCreate.invokeExact();

        if (app.equals(MemorySegment.NULL)) {
            throw new IllegalStateException(
                    "Raptor creation failed: " + getLastError()
            );
        }
    }

    public void run() throws Throwable {
        if (closed || app.equals(MemorySegment.NULL)) {
            throw new IllegalStateException(
                    "Create the Raptor application first"
            );
        }

        int result = (int) nativeRun.invokeExact(app);

        if (result != 0) {
            throw new IllegalStateException(
                    "Raptor run failed: " + getLastError()
            );
        }
    }

    public void destroy() throws Throwable {
        if (!app.equals(MemorySegment.NULL)) {
            MemorySegment oldApp = app;
            app = MemorySegment.NULL;
            nativeDestroy.invokeExact(oldApp);
        }
    }

    @Override
    public void close() throws Exception {
        if (closed) {
            return;
        }

        try {
            destroy();
        } catch (Throwable error) {
            throw new Exception("Failed to destroy Raptor", error);
        } finally {
            closed = true;
            libraryArena.close();
        }
    }
}