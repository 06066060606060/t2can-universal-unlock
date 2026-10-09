package dev.t2can.unlock;
/** One immediate retry per explicit/launch connection sequence. */
public final class ConnectionRetryPolicy {
    private boolean available;
    public void begin() { available = true; }
    public void cancel() { available = false; }
    public boolean take(boolean automatic) {
        if (!automatic || !available) return false;
        available = false;
        return true;
    }
}
