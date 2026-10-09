package dev.t2can.unlock;

public final class UrlPolicyTest {
    private static void check(boolean expected, String url) {
        if (UrlPolicy.isDevice(url) != expected) throw new AssertionError(url);
    }
    public static void main(String[] args) {
        check(true, "http://192.168.4.1/");
        check(true, "http://192.168.4.1:80/api/system/stats?test=1");
        check(true, "http://192.168.4.1/?resume=123&ota=1");
        check(false, "http://192.168.4.1.evil.example/");
        check(false, "http://192.168.4.1@evil.example/");
        check(false, "http://evil@192.168.4.1/");
        check(false, "http://192.168.4.1:8080/");
        check(false, "https://192.168.4.1/");
        check(false, "javascript:alert(1)");
        check(false, "file:///etc/passwd");
        check(false, "content://provider/file");
        check(false, "intent://example");
        check(false, "http://[::1]/");
        check(false, null);
        if (UrlPolicy.fileName("../../a.csv").contains("/")) throw new AssertionError("path traversal");
        if (!UrlPolicy.fileName("CAN_Research_Capture.csv").equals("CAN_Research_Capture.csv"))
            throw new AssertionError("log name changed");
        System.out.println("URL and filename policy: 16 checks PASS");
    }
}
