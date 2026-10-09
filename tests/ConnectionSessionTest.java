package dev.t2can.unlock;
public final class ConnectionSessionTest {
    static void check(boolean value) { if (!value) throw new AssertionError(); }
    public static void main(String[] args) {
        ConnectionSession s = new ConnectionSession();
        long first = s.begin(); check(s.accepts(first));
        check(s.complete(first)); check(!s.complete(first));
        long second = s.begin(); check(!s.accepts(first)); check(!s.complete(first));
        check(s.accepts(second)); s.cancel();
        check(!s.accepts(second)); check(!s.complete(second)); check(!s.isActive());
        long third = s.begin(); check(s.complete(third)); check(s.isReady());
        s.cancel(); s.cancel(); check(!s.isReady());
        long retry = s.begin();
        check(!s.accepts(third)); check(s.accepts(retry));
        check(s.complete(retry)); check(s.accepts(retry));
        s.cancel(); check(!s.complete(retry));
        System.out.println("ConnectionSessionTest: 17 checks PASS");
    }
}
