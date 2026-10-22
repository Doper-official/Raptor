public class Main {
    public static void main(String[] args) {
        try (Raptor raptor = new Raptor()) {
            System.out.println("Creating Raptor...");
            raptor.create();
            System.out.println("Running Raptor...");
            raptor.run();
            System.out.println("Raptor closed successfully.");
        } catch (Throwable error) {
            error.printStackTrace();
            // Important: let the launcher detect a failed startup/run.
            System.exit(1);
        }
    }
}
