package dev.t2can.unlock;

import java.net.URI;

/** Only the device origin can execute inside the privileged dashboard view. */
public final class UrlPolicy {
    public static final String HOME = "http://192.168.4.1/";
    public static boolean validHome(String home) {
        try {
            URI uri = new URI(home);
            String host = uri.getHost();
            if (uri.getRawUserInfo() != null || uri.getRawQuery() != null
                    || uri.getRawFragment() != null || uri.getPort() == 0 || uri.getPort() > 65535
                    || !(uri.getRawPath().isEmpty() || "/".equals(uri.getRawPath()))) return false;
            if ("http".equalsIgnoreCase(uri.getScheme())) return "192.168.4.1".equals(host);
            return "https".equalsIgnoreCase(uri.getScheme()) && privateIpv4(host);
        } catch (Exception ignored) { return false; }
    }
    private static boolean privateIpv4(String host) {
        if (host == null) return false;
        String[] parts = host.split("\\.", -1);
        if (parts.length != 4) return false;
        int[] ip = new int[4];
        try {
            for (int i=0; i<4; i++) {
                if (!parts[i].matches("0|[1-9][0-9]{0,2}")) return false;
                ip[i] = Integer.parseInt(parts[i]);
                if (ip[i] > 255) return false;
            }
        } catch (NumberFormatException invalid) { return false; }
        return ip[0] == 10 || (ip[0] == 172 && ip[1] >= 16 && ip[1] <= 31)
                || (ip[0] == 192 && ip[1] == 168);
    }
    private static int port(URI uri) {
        return uri.getPort() < 0 ? ("https".equalsIgnoreCase(uri.getScheme()) ? 443 : 80) : uri.getPort();
    }
    public static String origin(String home) {
        if (!validHome(home)) throw new IllegalArgumentException("Invalid dashboard address");
        URI uri = URI.create(home);
        return uri.getScheme().toLowerCase(java.util.Locale.ROOT) + "://" + uri.getRawAuthority();
    }
    public static boolean isDevice(String url) { return isDevice(url, HOME); }
    public static boolean isDevice(String url, String home) {
        try {
            if (!validHome(home)) return false;
            URI uri = new URI(url), base = new URI(home);
            return base.getScheme().equalsIgnoreCase(uri.getScheme())
                    && base.getHost().equals(uri.getHost()) && port(base) == port(uri)
                    && uri.getRawUserInfo() == null;
        } catch (Exception ignored) { return false; }
    }
    public static String fileName(String name) {
        String safe = name == null ? "" : name.replaceAll("[^a-zA-Z0-9._ -]", "_");
        safe = safe.replaceAll("^\\.+", "");
        if (safe.isEmpty()) safe = "T2CAN-log.csv";
        return safe.substring(0, Math.min(100, safe.length()));
    }
    private UrlPolicy() { }
}
