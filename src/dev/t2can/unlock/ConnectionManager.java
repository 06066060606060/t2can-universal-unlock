package dev.t2can.unlock;

import android.content.Context;
import android.content.pm.PackageManager;
import android.net.wifi.WifiInfo;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.LinkProperties;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSpecifier;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.Settings;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Owns local-network requests and process routing. All state lives on the main looper. */
public final class ConnectionManager {
    public interface Listener {
        void onState(String state, String detail);
        void onReady(Network network, DeviceProfile profile);
        void onDisconnected();
    }
    private final Context context;
    private final Listener listener;
    private final ConnectivityManager cm;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final ExecutorService io = Executors.newSingleThreadExecutor();
    private final ConnectionSession session = new ConnectionSession();
    private final ConnectionRetryPolicy retryPolicy = new ConnectionRetryPolicy();
    private final Set<Network> probing = new HashSet<>();
    private final Set<Network> lostNetworks = new HashSet<>();
    private ConnectivityManager.NetworkCallback requestCallback, defaultCallback;
    private Network bound, observedDefault;
    private DeviceProfile profile;
    private boolean closed;
    private boolean automaticMode;
    private String state = "IDLE", detail = "Choose a device to connect.";
    private String defaultRoute = "unknown";
    private volatile HttpURLConnection activeHttp;
    private long token;
    private Runnable deadline;
    private String localRoute = "unknown", linkState = "unknown", appVersion = "unknown";
    private long stateTimestamp, startedAt;
    private HealthResult lastHealth;
    private static final class HealthResult {
        final boolean ok;
        final int statusCode;
        final String error;
        final long durationMs;
        HealthResult(boolean ok, int statusCode, String error, long start) {
            this.ok = ok; this.statusCode = statusCode; this.error = error;
            durationMs = SystemClock.elapsedRealtime() - start;
        }
    }

    public ConnectionManager(Context context, Listener listener) {
        this.context = context.getApplicationContext(); this.listener = listener;
        cm = (ConnectivityManager) this.context.getSystemService(Context.CONNECTIVITY_SERVICE);
        try { appVersion = this.context.getPackageManager().getPackageInfo(this.context.getPackageName(), 0).versionName; }
        catch (PackageManager.NameNotFoundException ignored) { }
        defaultCallback = new ConnectivityManager.NetworkCallback() {
            @Override public void onAvailable(Network n) { observedDefault = n; }
            @Override public void onCapabilitiesChanged(Network n, NetworkCapabilities c) {
                observedDefault = n;
                defaultRoute = "network=" + n.getNetworkHandle() + ", " + (c.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) ? "cellular" :
                    c.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) ? "wifi" : "other") +
                    ", validated=" + c.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED) +
                    ", vpn=" + c.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
            }
            @Override public void onLost(Network n) { if (n.equals(observedDefault)) { observedDefault = null; defaultRoute = "unavailable"; } }
        };
        try { cm.registerDefaultNetworkCallback(defaultCallback, main); }
        catch (RuntimeException e) { defaultCallback = null; defaultRoute = "permission unavailable"; }
    }
    public boolean busy() { return session.isActive() && !session.isReady(); }
    public String state() { return state; }
    public String diagnostics() {
        HealthResult h = lastHealth;
        return "state=" + state + "\n" + detail + "\nstateTimeMs=" + stateTimestamp +
            "\nconnectionElapsedMs=" + (startedAt == 0 ? 0 : SystemClock.elapsedRealtime() - startedAt) +
            "\nboundNetwork=" + (bound == null ? "none" : bound.getNetworkHandle()) +
            "\nlocal=" + localRoute + "\nlink=" + linkState +
            "\nobservedDefault=" + defaultRoute +
            "\nhealth=" + (h == null ? "not checked" : "status=" + h.statusCode + ", result=" + h.error + ", elapsedMs=" + h.durationMs) +
            "\ndevice=" + Build.MANUFACTURER + " " + Build.MODEL + "\napp=" + appVersion +
            "\nSDK=" + Build.VERSION.SDK_INT + "; target=36" +
            "\nWebView / other-app Internet: NEEDS_DEVICE_TEST";
    }
    private void status(String next, String why) {
        state = next; detail = why; stateTimestamp = System.currentTimeMillis(); listener.onState(next, why);
    }
    private void capabilities(Network n, NetworkCapabilities c) {
        localRoute = "network=" + n.getNetworkHandle() + ", wifi=" + c.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) +
            ", internet=" + c.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) +
            ", validated=" + c.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED) +
            ", vpn=" + c.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
    }
    private void links(Network n, LinkProperties p) {
        linkState = "network=" + n.getNetworkHandle() + ", addresses=" + p.getLinkAddresses().size() +
            ", routes=" + p.getRoutes().size() +
            (Build.VERSION.SDK_INT >= 28 ? ", privateDns=" + p.isPrivateDnsActive() : "");
    }
    public void connect(DeviceProfile selected, boolean automatic) {
        if (closed) return;
        disconnect();
        retryPolicy.begin();
        connectAttempt(selected, automatic, false);
    }
    private void connectAttempt(DeviceProfile selected, boolean automatic, boolean retry) {
        if (closed) return;
        profile = selected; automaticMode = automatic; token = session.begin(); final long id = token;
        startedAt = SystemClock.elapsedRealtime(); lastHealth = null; localRoute = "unknown"; linkState = "unknown";
        status("CHECKING", "Checking the device on existing Wi-Fi.");
        deadline = () -> { if (session.accepts(id) && !session.isReady()) fail("Connection timed out. You can try manual mode."); };
        main.postDelayed(deadline, 40000);
        // A failed Android request must get a fresh request, not probes on its retiring network.
        if (retry) { request(id, automatic); return; }
        ArrayList<Network> candidates = new ArrayList<>();
        try {
            for (Network n : cm.getAllNetworks()) if (wifi(n) && ssidMatches(n, automatic)) candidates.add(n);
        } catch (RuntimeException e) { fail("Check network state permissions."); return; }
        checkExisting(id, candidates, 0, automatic);
    }
    private boolean wifi(Network n) {
        NetworkCapabilities c = cm.getNetworkCapabilities(n);
        return c != null && c.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) &&
            !c.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
    }
    private boolean ssidMatches(Network n, boolean requireVisible) {
        if (profile.ssid == null || profile.ssid.isEmpty()) return !requireVisible;
        NetworkCapabilities caps = cm.getNetworkCapabilities(n);
        if (Build.VERSION.SDK_INT >= 29 && caps != null && caps.getTransportInfo() instanceof WifiInfo) {
            String ssid = ((WifiInfo) caps.getTransportInfo()).getSSID();
            if (ssid != null && !"<unknown ssid>".equals(ssid)) {
                if (ssid.startsWith("\"") && ssid.endsWith("\"")) ssid = ssid.substring(1, ssid.length() - 1);
                return profile.ssid.equals(ssid);
            }
        }
        return !requireVisible;
    }
    private void checkExisting(long id, ArrayList<Network> networks, int index, boolean automatic) {
        if (!session.accepts(id)) return;
        if (index >= networks.size()) { request(id, automatic); return; }
        Network n = networks.get(index);
        probe(id, n, 0, () -> checkExisting(id, networks, index + 1, automatic));
    }
    private void request(long id, boolean automatic) {
        if (!session.accepts(id)) return;
        try {
            WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            if (wm == null || !wm.isWifiEnabled()) { fail("Turn Wi-Fi on and connect again."); return; }
            NetworkRequest.Builder b = new NetworkRequest.Builder().addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
            if (automatic) {
                if (Build.VERSION.SDK_INT < 29) { fail("Connect manually in Wi-Fi settings on this Android version."); return; }
                if (Settings.Global.getInt(context.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, 0) != 0) {
                    fail("Turn airplane mode off and connect again."); return;
                }
                if (Build.VERSION.SDK_INT <= 32) {
                    LocationManager lm = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
                    if (lm == null || !lm.isLocationEnabled()) { fail("Enable Location settings for automatic Wi-Fi connection."); return; }
                }
                WifiNetworkSpecifier.Builder w = new WifiNetworkSpecifier.Builder().setSsid(profile.ssid);
                if ("WPA2".equals(profile.security)) w.setWpa2Passphrase(profile.password);
                else if ("WPA3".equals(profile.security)) w.setWpa3Passphrase(profile.password);
                b.setNetworkSpecifier(w.build());
            }
            requestCallback = new ConnectivityManager.NetworkCallback() {
                @Override public void onAvailable(Network n) {
                    if (!session.accepts(id) || session.isReady() || probing.contains(n)) return;
                    try { if (!ssidMatches(n, false)) return; }
                    catch (RuntimeException e) { fail("Unable to read Wi-Fi state. Check app permissions."); return; }
                    lostNetworks.remove(n); probing.add(n); probe(id, n, 0, () -> {
                        probing.remove(n);
                        if (automatic && session.accepts(id)) fail("Wi-Fi connected, but the device did not provide a valid response.");
                    });
                }
                @Override public void onCapabilitiesChanged(Network n, NetworkCapabilities c) {
                    if (session.accepts(id) && (bound == null || bound.equals(n))) capabilities(n, c);
                }
                @Override public void onLinkPropertiesChanged(Network n, LinkProperties p) {
                    if (session.accepts(id) && (bound == null || bound.equals(n))) links(n, p);
                }
                @Override public void onLost(Network n) {
                    if (!session.accepts(id)) return;
                    lostNetworks.add(n); probing.remove(n);
                    if (n.equals(bound)) fail("Device Wi-Fi disconnected. Connect again.");
                }
                @Override public void onUnavailable() {
                    if (!session.accepts(id) || session.isReady()) return;
                    if (retryPolicy.take(automatic)) {
                        DeviceProfile retryProfile = profile;
                        teardown();
                        connectAttempt(retryProfile, true, true);
                    } else fail("Connection was not approved or the device could not be reached. Tap Connect to try again.");
                }
            };
            status("CONNECTING", automatic ? "Waiting for Android Wi-Fi connection." : "Waiting for the device on manually connected Wi-Fi.");
            if (automatic) cm.requestNetwork(b.build(), requestCallback, main, 35000);
            else cm.registerNetworkCallback(b.build(), requestCallback, main);
        } catch (SecurityException e) { fail("Wi-Fi connection permission is missing. Check app permissions."); }
        catch (RuntimeException e) { fail("This device could not process the connection request. Try manual mode."); }
    }
    private void probe(long id, Network n, int attempt, Runnable exhausted) {
        if (!session.accepts(id) || session.isReady() || lostNetworks.contains(n)) return;
        final String home = profile.url;
        status("VERIFYING", "Checking the device response (attempt " + (attempt + 1) + "/3).");
        try {
            NetworkCapabilities caps = cm.getNetworkCapabilities(n);
            LinkProperties lp = cm.getLinkProperties(n);
            if (caps != null) capabilities(n, caps);
            if (lp != null) links(n, lp);
        } catch (RuntimeException ignored) { }
        io.execute(() -> {
            if (!session.accepts(id)) return;
            HealthResult result = health(id, n, home);
            main.post(() -> {
                if (!session.accepts(id) || session.isReady() || lostNetworks.contains(n)) return;
                lastHealth = result;
                if (result.ok) { ready(id, n); return; }
                if (attempt < 2) main.postDelayed(() -> probe(id, n, attempt + 1, exhausted), (attempt + 1) * 1000L);
                else exhausted.run();
            });
        });
    }
    private HealthResult health(long id, Network network, String home) {
        HttpURLConnection connection = null;
        final long start = SystemClock.elapsedRealtime();
        final long expires = start + 7000;
        int code = -1;
        try {
            if (!session.accepts(id)) return new HealthResult(false, code, "cancelled", start);
            connection = (HttpURLConnection) network.openConnection(new URL(UrlPolicy.origin(home) + "/api/profile/status"));
            activeHttp = connection;
            if (!session.accepts(id)) return new HealthResult(false, code, "cancelled", start);
            connection.setConnectTimeout(2500); connection.setReadTimeout(2500);
            connection.setInstanceFollowRedirects(false); connection.setUseCaches(false);
            code = connection.getResponseCode();
            if (code != 200) return new HealthResult(false, code, "http_status", start);
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            try (InputStream in = connection.getInputStream()) {
                byte[] buffer = new byte[1024]; int count;
                while ((count = in.read(buffer)) != -1) {
                    if (!session.accepts(id) || Thread.currentThread().isInterrupted()) return new HealthResult(false, code, "cancelled", start);
                    if (bytes.size() + count > 16384) return new HealthResult(false, code, "response_too_large", start);
                    if (SystemClock.elapsedRealtime() > expires) return new HealthResult(false, code, "read_deadline", start);
                    bytes.write(buffer, 0, count);
                }
            }
            JSONObject json = new JSONObject(bytes.toString("UTF-8"));
            boolean ok = json.has("setupMode") && json.has("profile");
            return new HealthResult(ok, code, ok ? "ok" : "invalid_device_response", start);
        } catch (java.net.SocketTimeoutException e) { return new HealthResult(false, code, "timeout", start); }
        catch (javax.net.ssl.SSLException e) { return new HealthResult(false, code, "tls", start); }
        catch (SecurityException e) { return new HealthResult(false, code, "permission", start); }
        catch (org.json.JSONException e) { return new HealthResult(false, code, "invalid_json", start); }
        catch (Exception e) { return new HealthResult(false, code, "connection", start); }
        finally { if (connection != null) connection.disconnect(); if (activeHttp == connection) activeHttp = null; }
    }
    private void ready(long id, Network network) {
        if (!session.accepts(id) || session.isReady()) return;
        try {
            if (!wifi(network) || !cm.bindProcessToNetwork(network) || !network.equals(cm.getBoundNetworkForProcess())) {
                fail("Unable to bind this app to the device network."); return;
            }
            bound = network;
            if (!session.complete(id)) return;
            if (deadline != null) main.removeCallbacks(deadline);
            // Existing networks also need a loss observer.
            if (requestCallback == null) {
                requestCallback = new ConnectivityManager.NetworkCallback() {
                    @Override public void onCapabilitiesChanged(Network n, NetworkCapabilities c) {
                        if (session.accepts(id) && n.equals(bound)) capabilities(n, c);
                    }
                    @Override public void onLinkPropertiesChanged(Network n, LinkProperties p) {
                        if (session.accepts(id) && n.equals(bound)) links(n, p);
                    }
                    @Override public void onLost(Network n) { if (session.accepts(id) && n.equals(bound)) fail("Device Wi-Fi disconnected."); }
                };
                cm.registerNetworkCallback(new NetworkRequest.Builder().addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                    .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET).build(), requestCallback, main);
            }
            status("DASHBOARD", "Device response and app route verified. Loading dashboard.");
            listener.onReady(network, profile);
        } catch (RuntimeException e) { fail("Device network changed. Connect again."); }
    }
    private void fail(String why) { retryPolicy.cancel(); teardown(); status("ERROR", why); }
    public void disconnect() { retryPolicy.cancel(); teardown(); if (!closed) status("IDLE", "Disconnected."); }
    private void teardown() {
        session.cancel(); listener.onDisconnected();
        main.removeCallbacksAndMessages(null);
        if (activeHttp != null) activeHttp.disconnect();
        if (requestCallback != null) {
            try { cm.unregisterNetworkCallback(requestCallback); } catch (RuntimeException ignored) { }
            requestCallback = null;
        }
        try { cm.bindProcessToNetwork(null); } catch (RuntimeException ignored) { }
        bound = null; probing.clear(); lostNetworks.clear();
    }
    public void resume() {
        if (bound == null || !session.isActive()) return;
        if (automaticMode) {
            String permission = Build.VERSION.SDK_INT >= 33 ? "android.permission.NEARBY_WIFI_DEVICES" : "android.permission.ACCESS_FINE_LOCATION";
            if (context.checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                fail("Wi-Fi permission changed. Check app permissions."); return;
            }
        }
        try { if (!wifi(bound) || !bound.equals(cm.getBoundNetworkForProcess())) fail("Connection state changed. Connect again."); }
        catch (RuntimeException e) { fail("Check connection permissions."); }
    }
    public void close() {
        if (closed) return;
        disconnect(); closed = true;
        if (defaultCallback != null) { try { cm.unregisterNetworkCallback(defaultCallback); } catch (RuntimeException ignored) { } defaultCallback = null; }
        io.shutdownNow();
    }
}
