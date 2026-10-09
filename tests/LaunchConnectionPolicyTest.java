package dev.t2can.unlock;
public final class LaunchConnectionPolicyTest {
    private static int checks;
    private static void check(boolean result,String why) { checks++;if(!result)throw new AssertionError(why); }
    public static void main(String[] args) {
        DeviceProfile registered=new DeviceProfile("id","T2CAN","T2CAN-example","WPA2","example-1234",UrlPolicy.HOME);
        LaunchConnectionPolicy launch=new LaunchConnectionPolicy();
        check(launch.onForeground(registered,36,true)==LaunchConnectionPolicy.Action.CONNECT,"Registered device must connect on launch without tapping Connect");
        check(launch.onForeground(registered,36,true)==LaunchConnectionPolicy.Action.NONE,"No duplicate on resume");
        check(new LaunchConnectionPolicy().onForeground(registered,36,true)==LaunchConnectionPolicy.Action.CONNECT,"Fresh launch retries previous failure/disconnect");
        LaunchConnectionPolicy cancelled=new LaunchConnectionPolicy();cancelled.consume();
        check(cancelled.onForeground(registered,36,true)==LaunchConnectionPolicy.Action.NONE,"Explicit interaction consumes automatic request");
        LaunchConnectionPolicy permission=new LaunchConnectionPolicy();
        check(permission.onForeground(registered,36,false)==LaunchConnectionPolicy.Action.NEEDS_PERMISSION,"Missing permission must show action, not an automatic permission popup");
        check(permission.onForeground(registered,36,true)==LaunchConnectionPolicy.Action.NONE,"Permission/settings return must not restart automatically");
        check(new LaunchConnectionPolicy().onForeground(DeviceProfile.legacy(),36,true)==LaunchConnectionPolicy.Action.NONE,"No guessed SSID for unregistered device");
        check(new LaunchConnectionPolicy().onForeground(registered,28,true)==LaunchConnectionPolicy.Action.NONE,"No unsupported automatic API");
        check(new LaunchConnectionPolicy().onForeground(null,36,true)==LaunchConnectionPolicy.Action.NONE,"No selected device");
        DeviceProfile invalid=new DeviceProfile("id","T2CAN","test","WPA2","bad",UrlPolicy.HOME);
        check(new LaunchConnectionPolicy().onForeground(invalid,36,true)==LaunchConnectionPolicy.Action.NONE,"Do not auto-open editor on invalid saved profile");
        LaunchConnectionPolicy empty=new LaunchConnectionPolicy();empty.onForeground(DeviceProfile.legacy(),36,true);
        check(empty.onForeground(registered,36,true)==LaunchConnectionPolicy.Action.NONE,"Saving a profile during the current launch does not trigger unexpected connection");
        check(new LaunchConnectionPolicy().onForeground(registered,29,true)==LaunchConnectionPolicy.Action.CONNECT,"Android10 supports launch connect");
        System.out.println("LaunchConnectionPolicyTest: "+checks+" checks PASS");
    }
}
