package dev.t2can.unlock;
import java.lang.reflect.Method;
public final class ProfileUrlPolicyTest {
    private static int checks;
    // Exercise the old single-origin behavior before the configurable API exists.
    private static boolean allows(String url, String home) throws Exception {
        try { return (Boolean) UrlPolicy.class.getMethod("isDevice", String.class, String.class).invoke(null,url,home); }
        catch (NoSuchMethodException old) { return UrlPolicy.isDevice(url); }
    }
    private static void check(boolean expected, String url,String home) throws Exception {
        checks++; if (allows(url,home)!=expected) throw new AssertionError("origin policy: "+url+" for "+home);
    }
    public static void main(String[] args) throws Exception {
        String home="https://10.0.0.2:8443/";
        check(true,home+"api/profile/status",home);
        check(false,"http://10.0.0.2:8443/",home);
        check(false,"https://10.0.0.2/",home);
        check(false,"https://user@10.0.0.2:8443/",home);
        check(false,"https://10.0.0.2.evil:8443/",home);
        check(false,"http://192.168.4.1/",home);
        check(true,"http://192.168.4.1:8080/api?a=1","http://192.168.4.1:8080/");
        Method valid=UrlPolicy.class.getMethod("validHome",String.class);
        for(String s:new String[]{UrlPolicy.HOME,home,"https://172.16.0.1/","https://192.168.1.1/"}) {
            checks++; if(!(Boolean)valid.invoke(null,s)) throw new AssertionError("valid home rejected: "+s);
        }
        for(String s:new String[]{"http://10.0.0.1/","https://example.com/","https://8.8.8.8/","https://127.0.0.1/","https://172.32.0.1/","https://192.168.1.1/a","https://192.168.1.1/?token=x","https://192.168.1.1/#x","https://u@192.168.1.1/","https://192.168.1.1:0/","https://192.168.1.1:65536/","https://192.168.001.1/",null}) {
            checks++; if((Boolean)valid.invoke(null,s)) throw new AssertionError("unsafe home allowed: "+s);
        }
        System.out.println("Profile URL policy: "+checks+" checks PASS");
    }
}
