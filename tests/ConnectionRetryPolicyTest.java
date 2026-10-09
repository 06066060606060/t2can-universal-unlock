package dev.t2can.unlock;
public final class ConnectionRetryPolicyTest {
    private static int checks;
    private static void check(boolean ok, String why) { checks++; if (!ok) throw new AssertionError(why); }
    public static void main(String[] args) {
        ConnectionRetryPolicy p = new ConnectionRetryPolicy();
        check(!p.take(true), "No retry without a connection sequence");
        p.begin();
        check(p.take(true), "First unavailable must immediately allow retry");
        check(!p.take(true), "Second unavailable must stop");
        check(!p.take(true), "Repeated callbacks cannot create a retry loop");
        p.begin(); p.cancel();
        check(!p.take(true), "Disconnect cancels retry");
        p.begin(); check(!p.take(false), "Manual Wi-Fi mode never requests automatic retry");
        p.begin(); check(p.take(true), "New explicit connect resets budget");
        p.cancel(); check(!p.take(true), "Close/failure cancels remaining budget");
        ConnectionSession s = new ConnectionSession(); long old = s.begin();
        s.cancel(); long next = s.begin();
        check(!s.accepts(old), "Old network callbacks cannot affect retry");
        check(s.accepts(next), "Retry owns the current generation");
        check(s.complete(next), "Retry can reach dashboard");
        check(!s.complete(old), "Old completion cannot replace retry connection");
        System.out.println("ConnectionRetryPolicyTest: " + checks + " checks PASS");
    }
}
