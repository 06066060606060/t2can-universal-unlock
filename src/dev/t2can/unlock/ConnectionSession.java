package dev.t2can.unlock;

/** Pure generation gate for asynchronous network ownership. */
public final class ConnectionSession {
    private long generation;
    private boolean active;
    private boolean ready;
    public synchronized long begin() { generation++; active = true; ready = false; return generation; }
    public synchronized void cancel() { generation++; active = false; ready = false; }
    public synchronized boolean accepts(long token) { return active && token == generation; }
    public synchronized boolean complete(long token) {
        if (!accepts(token) || ready) return false;
        ready = true;
        return true;
    }
    public synchronized boolean isActive() { return active; }
    public synchronized boolean isReady() { return ready; }
}
