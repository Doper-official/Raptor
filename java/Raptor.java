import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.net.URISyntaxException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

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
        System.out.println("[RAPTOR] Native DLL: " + dllPath);
        library = SymbolLookup.libraryLookup(dllPath, libraryArena);
        nativeCreate = bind("raptor_first_app_create", FunctionDescriptor.of(ValueLayout.ADDRESS));
        nativeRun = bind("raptor_first_app_run", FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.ADDRESS));
        nativeDestroy = bind("raptor_first_app_destroy", FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
        nativeLastError = bind("raptor_last_error", FunctionDescriptor.of(ValueLayout.ADDRESS));
    }

    private MethodHandle bind(String name, FunctionDescriptor descriptor) {
        MemorySegment address = library.find(name).orElseThrow(
            () -> new UnsatisfiedLinkError("Missing native symbol: " + name));
        return LINKER.downcallHandle(address, descriptor);
    }

    private static Path applicationDirectory() throws Exception {
        try {
            Path location = Path.of(Raptor.class.getProtectionDomain()
                .getCodeSource().getLocation().toURI()).toAbsolutePath().normalize();
            // When launched with -jar, code source points to the JAR itself.
            return Files.isDirectory(location) ? location : location.getParent();
        } catch (URISyntaxException | NullPointerException ex) {
            return Path.of("").toAbsolutePath().normalize();
        }
    }

    private static Path findDll() throws Exception {
        List<Path> candidates = new ArrayList<>();
        Path appDir = applicationDirectory();
        candidates.add(appDir.resolve("raptor.dll"));
        candidates.add(appDir.resolve("native").resolve("raptor.dll"));
        // Useful when running from an IDE/development working directory.
        candidates.add(Path.of("").toAbsolutePath().normalize().resolve("raptor.dll"));

        for (Path candidate : candidates) {
            Path normalized = candidate.toAbsolutePath().normalize();
            if (Files.isRegularFile(normalized)) {
                return normalized;
            }
        }

        StringBuilder searched = new StringBuilder();
        for (Path candidate : candidates) {
            searched.append("\n - ").append(candidate.toAbsolutePath().normalize());
        }
        throw new IllegalStateException("Could not find raptor.dll. Searched:" + searched);
    }

    private String getLastError() throws Throwable {
        MemorySegment error = (MemorySegment) nativeLastError.invokeExact();
        if (error.equals(MemorySegment.NULL)) return "Unknown native error";
        return error.reinterpret(4096).getString(0, StandardCharsets.UTF_8);
    }

    public void create() throws Throwable {
        if (closed) throw new IllegalStateException("Raptor is closed");
        if (!app.equals(MemorySegment.NULL)) throw new IllegalStateException("Raptor application already created");
        app = (MemorySegment) nativeCreate.invokeExact();
        if (app.equals(MemorySegment.NULL)) throw new IllegalStateException("Raptor creation failed: " + getLastError());
    }

    public void run() throws Throwable {
        if (closed || app.equals(MemorySegment.NULL)) throw new IllegalStateException("Create the Raptor application first");
        int result = (int) nativeRun.invokeExact(app);
        if (result != 0) throw new IllegalStateException("Raptor run failed: " + getLastError());
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
        if (closed) return;
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
