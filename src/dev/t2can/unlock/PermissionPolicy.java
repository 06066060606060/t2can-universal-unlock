package dev.t2can.unlock;

/** Permission needed by an automatic local-only Wi-Fi request; manual mode needs none. */
public final class PermissionPolicy {
    public static String requiredPermission(int sdk, int target) {
        if (sdk < 29) return null;
        return sdk >= 33 && target >= 33 ? "android.permission.NEARBY_WIFI_DEVICES"
                : "android.permission.ACCESS_FINE_LOCATION";
    }
    public static String[] requestPermissions(int sdk, int target) {
        String permission=requiredPermission(sdk,target);
        if ("android.permission.ACCESS_FINE_LOCATION".equals(permission) && sdk >= 31)
            return new String[]{permission,"android.permission.ACCESS_COARSE_LOCATION"};
        return permission==null?new String[0]:new String[]{permission};
    }
    private PermissionPolicy() { }
}
