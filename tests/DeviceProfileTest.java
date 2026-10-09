package dev.t2can.unlock;
public final class DeviceProfileTest {
    static int checks;
    static void check(boolean ok, String name) { checks++; if (!ok) throw new AssertionError(name); }
    static DeviceProfile p(String ssid,String security,String password) {
        return new DeviceProfile("1","Device",ssid,security,password,UrlPolicy.HOME);
    }
    public static void main(String[] args) {
        check(DeviceProfile.legacy().validationError(false)==null,"legacy manual supported");
        check(DeviceProfile.legacy().validationError(true)!=null,"automatic requires actual SSID");
        check(p("T2CAN-1234","WPA2","12345678").validationError(true)==null,"WPA2 accepted");
        check(p("T2CAN-1234","WPA3","12345678").validationError(true)==null,"WPA3 accepted");
        check(p("AP","OPEN","").validationError(true)==null,"open accepted");
        check(p("AP","OPEN","secret").validationError(true)!=null,"open credentials rejected");
        check(p("AP","WEP","12345678").validationError(true)!=null,"unsupported security rejected");
        check(p("AP","WPA2","short").validationError(true)!=null,"short WPA2 rejected");
        check(p("AP","WPA2",String.join("",java.util.Collections.nCopies(64,"a"))).validationError(true)!=null,"64 char pass rejected");
        check(p("AP","WPA2","1234567é").validationError(true)!=null,"non ASCII passphrase rejected");
        check(p(String.join("",java.util.Collections.nCopies(11,"한")),"OPEN","").validationError(true)!=null,"UTF8 SSID bytes capped");
        check(p("AP\n","OPEN","").validationError(true)!=null,"control SSID rejected");
        System.out.println("DeviceProfileTest: "+checks+" checks PASS");
    }
}
