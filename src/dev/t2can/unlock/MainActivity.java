package dev.t2can.unlock;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.webkit.JsResult;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebMessage;
import android.webkit.WebMessagePort;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.TextView;
import android.widget.Toast;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.json.JSONObject;

public final class MainActivity extends Activity {
    private static final int FILE_REQUEST = 101;
    private final Handler main = new Handler(Looper.getMainLooper());

    private ConnectionManager manager;
    private LaunchConnectionPolicy launchConnection = new LaunchConnectionPolicy();
    private ProfileRepository profiles;
    private ConnectionPanel panel;
    private DeviceProfile pendingPermission;
    private String dashboardHome = UrlPolicy.HOME;
    private String webResult = "not loaded";
    private Button devicesButton, menuButton;
    private boolean showingSettings;
    private static final int WIFI_PERMISSION = 103;
    private WebView web;
    private FrameLayout root;
    private FrameLayout connectionScreen;
    private LinearLayout connectionCard, connectionStatusChip;
    private TextView connectionHeader, connectionMessage, connectionStatus;
    private TextView connectionLabel, connectionFooter;
    private Button wifiSettingsButton, retryButton;
    private ValueCallback<Uri[]> fileCallback;
    private FileExporter exporter;
    private String bridge;
    private boolean destroyed, loaded, loadStarted, dashboardLoadedOnce;
    private int documentEpoch;
    private final Runnable loadTimeout = () -> { if(loadStarted && !destroyed) pageFailed(); };

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        Object retained = getLastNonConfigurationInstance();
        if (retained instanceof LaunchConnectionPolicy) launchConnection = (LaunchConnectionPolicy) retained;
        try { profiles = new ProfileRepository(this); }
        catch (Exception error) {
            new AlertDialog.Builder(this).setTitle("Secure profiles unavailable")
                .setMessage("Saved profiles could not be decrypted. Existing data has not been overwritten. Restore access to the original Android Keystore or reset app data in Android settings to register devices again.")
                .setPositiveButton("Close",(d,w)->finish()).setOnCancelListener(d->finish()).show();
            return;
        }
        bridge = readAsset("dashboard-bridge.js");
        createUi();
        configureInsets();
        updateTheme();
        if (Build.VERSION.SDK_INT >= 33) {
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                    android.window.OnBackInvokedDispatcher.PRIORITY_DEFAULT, this::dashboardBack);
        }
        manager = new ConnectionManager(this, new ConnectionManager.Listener() {
            public void onState(String state, String detail) {
                if(destroyed) return;
                connectionStatus.setText(state.replace('_',' '));
                connectionMessage.setText(detail);
                wifiSettingsButton.setEnabled(!manager.busy());
                retryButton.setEnabled(!manager.busy());
            }
            public void onReady(Network network, DeviceProfile profile) { openDashboard(profile); }
            public void onDisconnected() { destroyDashboard(); }
        });
        panel = new ConnectionPanel(this, profiles, new ConnectionPanel.Actions() {
            public void connect(boolean automatic) { startConnection(automatic); }
            public void disconnect() { disconnectDevice(); }
            public void profileChanged() { refreshProfile(); }
            public String diagnostics() { return manager.diagnostics()+"\nWebView: "+webResult; }
        });
        refreshProfile();
    }

    private int dp(int n) { return Math.round(n * getResources().getDisplayMetrics().density); }

    private void createUi() {
        root = new FrameLayout(this);

        connectionScreen = new FrameLayout(this);
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setVerticalScrollBarEnabled(false);
        LinearLayout page = new LinearLayout(this);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setPadding(dp(20), dp(22), dp(20), dp(20));

        LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        connectionHeader = new TextView(this);
        connectionHeader.setText(R.string.header_title);
        connectionHeader.setTextSize(15);
        connectionHeader.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        connectionHeader.setIncludeFontPadding(false);
        connectionHeader.setGravity(Gravity.CENTER_VERTICAL);
        header.addView(connectionHeader, new LinearLayout.LayoutParams(0, dp(44), 1f));

        connectionStatusChip = new LinearLayout(this);
        connectionStatusChip.setOrientation(LinearLayout.HORIZONTAL);
        connectionStatusChip.setGravity(Gravity.CENTER);
        connectionStatusChip.setPadding(dp(12), 0, dp(14), 0);
        View statusDot = new View(this);
        statusDot.setBackgroundResource(R.drawable.connection_status_dot);
        LinearLayout.LayoutParams dotLayout = new LinearLayout.LayoutParams(dp(8), dp(8));
        dotLayout.rightMargin = dp(8);
        connectionStatusChip.addView(statusDot, dotLayout);
        connectionStatus = new TextView(this);
        connectionStatus.setText(R.string.status_disconnected);
        connectionStatus.setTextSize(14);
        connectionStatus.setIncludeFontPadding(false);
        connectionStatusChip.addView(connectionStatus);
        header.addView(connectionStatusChip, new LinearLayout.LayoutParams(-2, dp(42)));
        page.addView(header, new LinearLayout.LayoutParams(-1, dp(44)));

        page.addView(new Space(this), new LinearLayout.LayoutParams(dp(1), 0, 1f));

        connectionCard = new LinearLayout(this);
        connectionCard.setOrientation(LinearLayout.VERTICAL);
        connectionCard.setGravity(Gravity.CENTER_HORIZONTAL);
        connectionCard.setPadding(dp(24), dp(28), dp(24), dp(24));
        ImageView icon = new ImageView(this);
        icon.setImageResource(R.mipmap.ic_launcher);
        icon.setContentDescription(getString(R.string.app_name));
        connectionCard.addView(icon, new LinearLayout.LayoutParams(dp(80), dp(80)));

        connectionLabel = new TextView(this);
        connectionLabel.setText(R.string.connection_label);
        connectionLabel.setTextSize(12);
        connectionLabel.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        connectionLabel.setLetterSpacing(.08f);
        connectionLabel.setGravity(Gravity.CENTER);
        connectionLabel.setIncludeFontPadding(false);
        LinearLayout.LayoutParams labelLayout = new LinearLayout.LayoutParams(-1, -2);
        labelLayout.topMargin = dp(22);
        connectionCard.addView(connectionLabel, labelLayout);

        connectionMessage = new TextView(this);
        connectionMessage.setTextSize(20);
        connectionMessage.setLineSpacing(dp(3), 1f);
        connectionMessage.setGravity(Gravity.CENTER);
        connectionMessage.setText(R.string.connection_initial);
        connectionMessage.setIncludeFontPadding(false);
        LinearLayout.LayoutParams messageLayout = new LinearLayout.LayoutParams(-1, -2);
        messageLayout.topMargin = dp(12);
        connectionCard.addView(connectionMessage, messageLayout);

        wifiSettingsButton = new Button(this);
        prepareConnectionButton(wifiSettingsButton);
        wifiSettingsButton.setText("Connect");
        wifiSettingsButton.setOnClickListener(v -> startConnection(true));
        LinearLayout.LayoutParams primaryLayout = new LinearLayout.LayoutParams(-1, dp(56));
        primaryLayout.topMargin = dp(26);
        connectionCard.addView(wifiSettingsButton, primaryLayout);

        retryButton = new Button(this);
        prepareConnectionButton(retryButton);
        retryButton.setText("Manual connection");
        retryButton.setOnClickListener(v -> panel.manual());
        LinearLayout.LayoutParams retryLayout = new LinearLayout.LayoutParams(-1, dp(56));
        retryLayout.topMargin = dp(10);
        connectionCard.addView(retryButton, retryLayout);

        devicesButton = new Button(this);
        prepareConnectionButton(devicesButton); devicesButton.setText("Devices / connection settings");
        devicesButton.setOnClickListener(v -> panel.show());
        LinearLayout.LayoutParams deviceLayout = new LinearLayout.LayoutParams(-1,dp(56));
        deviceLayout.topMargin=dp(10);connectionCard.addView(devicesButton,deviceLayout);
        page.addView(connectionCard, new LinearLayout.LayoutParams(-1, -2));
        page.addView(new Space(this), new LinearLayout.LayoutParams(dp(1), 0, 1f));

        connectionFooter = new TextView(this);
        connectionFooter.setText(R.string.endpoint_version);
        connectionFooter.setTextSize(13);
        connectionFooter.setGravity(Gravity.CENTER);
        connectionFooter.setIncludeFontPadding(false);
        LinearLayout.LayoutParams footerLayout = new LinearLayout.LayoutParams(-1, dp(42));
        footerLayout.topMargin = dp(12);
        page.addView(connectionFooter, footerLayout);

        scroll.addView(page, new ScrollView.LayoutParams(-1, -1));
        connectionScreen.addView(scroll, new FrameLayout.LayoutParams(-1, -1));
        root.addView(connectionScreen, new FrameLayout.LayoutParams(-1, -1));
        menuButton=new Button(this);prepareConnectionButton(menuButton);menuButton.setText("Connection");
        menuButton.setOnClickListener(v->showConnectionMenu());menuButton.setVisibility(View.GONE);
        FrameLayout.LayoutParams menuLayout=new FrameLayout.LayoutParams(-1,dp(40),Gravity.BOTTOM);
        root.addView(menuButton,menuLayout);
        setContentView(root);
    }

    private void prepareConnectionButton(Button button) {
        button.setAllCaps(false);
        button.setTextSize(15);
        button.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        button.setGravity(Gravity.CENTER);
        button.setMinHeight(0);
        button.setMinimumHeight(0);
        button.setPadding(dp(16), 0, dp(16), 0);
        button.setElevation(0);
        button.setStateListAnimator(null);
        button.setBackgroundTintList(null);
    }

    private GradientDrawable roundedBackground(int color, int radius, int strokeWidth, int strokeColor) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(color);
        drawable.setCornerRadius(dp(radius));
        if (strokeWidth > 0) drawable.setStroke(dp(strokeWidth), strokeColor);
        return drawable;
    }

    private RippleDrawable buttonBackground(int color, int strokeWidth, int strokeColor) {
        GradientDrawable content = roundedBackground(color, 18, strokeWidth, strokeColor);
        GradientDrawable mask = roundedBackground(Color.WHITE, 18, 0, Color.TRANSPARENT);
        return new RippleDrawable(ColorStateList.valueOf(getColor(R.color.connection_ripple)), content, mask);
    }

    private void configureInsets() {
        if (Build.VERSION.SDK_INT >= 30) {
            getWindow().setDecorFitsSystemWindows(false);
            root.setOnApplyWindowInsetsListener((v, insets) -> {
                android.graphics.Insets bars = insets.getInsets(WindowInsets.Type.systemBars()
                        | WindowInsets.Type.displayCutout() | WindowInsets.Type.ime());
                v.setPadding(bars.left, bars.top, bars.right, bars.bottom);
                return WindowInsets.CONSUMED;
            });
        }
    }

    private boolean isDark() {
        return (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK)
                == Configuration.UI_MODE_NIGHT_YES;
    }

    private void updateTheme() {
        int color = getColor(R.color.connection_background);
        root.setBackgroundColor(color);
        if(web!=null) web.setBackgroundColor(color);
        connectionScreen.setBackgroundColor(color);
        connectionHeader.setTextColor(getColor(R.color.connection_text));
        connectionStatus.setTextColor(getColor(R.color.connection_text));
        connectionLabel.setTextColor(getColor(R.color.connection_muted));
        connectionMessage.setTextColor(getColor(R.color.connection_text));
        connectionFooter.setTextColor(getColor(R.color.connection_muted));
        connectionCard.setBackground(roundedBackground(getColor(R.color.connection_card), 28, 0, Color.TRANSPARENT));
        connectionStatusChip.setBackground(roundedBackground(getColor(R.color.connection_chip), 999, 0, Color.TRANSPARENT));
        wifiSettingsButton.setBackground(buttonBackground(getColor(R.color.connection_primary), 0, Color.TRANSPARENT));
        wifiSettingsButton.setTextColor(getColor(R.color.connection_primary_text));
        retryButton.setBackground(buttonBackground(getColor(R.color.connection_secondary), 1,
                getColor(R.color.connection_secondary_stroke)));
        retryButton.setTextColor(getColor(R.color.connection_secondary_text));
        devicesButton.setBackground(buttonBackground(getColor(R.color.connection_secondary),1,getColor(R.color.connection_secondary_stroke)));
        devicesButton.setTextColor(getColor(R.color.connection_secondary_text));
        menuButton.setBackgroundColor(getColor(R.color.connection_card));
        menuButton.setTextColor(getColor(R.color.connection_text));
        getWindow().setStatusBarColor(color);
        getWindow().setNavigationBarColor(color);
        if (Build.VERSION.SDK_INT >= 30) {
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                int light = WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS
                        | WindowInsetsController.APPEARANCE_LIGHT_NAVIGATION_BARS;
                controller.setSystemBarsAppearance(isDark() ? 0 : light, light);
            }
        } else {
            getWindow().getDecorView().setSystemUiVisibility(isDark() ? 0
                    : View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR | View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR);
        }
        if (loaded && web != null) web.evaluateJavascript("window.__tuNativeTheme?.(" + isDark() + ")", null);
    }

    private void configureWebView() {
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(false);
        settings.setAllowContentAccess(true); // User-selected OTA content URI only.
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        settings.setCacheMode(WebSettings.LOAD_NO_CACHE);
        settings.setSupportMultipleWindows(false);
        settings.setJavaScriptCanOpenWindowsAutomatically(false);
        settings.setMediaPlaybackRequiresUserGesture(true);
        web.setWebViewClient(new WebViewClient() {
            @Override public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                return !UrlPolicy.isDevice(request.getUrl().toString(), dashboardHome);
            }
            @Override public WebResourceResponse shouldInterceptRequest(WebView view, WebResourceRequest request) {
                if (UrlPolicy.isDevice(request.getUrl().toString(), dashboardHome)) return null;
                return new WebResourceResponse("text/plain", "UTF-8", 403, "Blocked",
                        java.util.Collections.emptyMap(), new ByteArrayInputStream(new byte[0]));
            }
            @Override public void onPageStarted(WebView view, String url, android.graphics.Bitmap favicon) {
                if(destroyed || view != web) return;
                documentEpoch++;
                loaded = false;
                loadStarted = true;
                closeExporter();
            }
            @Override public void onPageFinished(WebView view, String url) {
                if (view != web || !loadStarted || !UrlPolicy.isDevice(url, dashboardHome)) return;
                final int epoch = documentEpoch;
                web.evaluateJavascript(bridge, ignored -> {
                    if (destroyed || view != web || epoch != documentEpoch || !loadStarted) return;
                    WebMessagePort[] ports = web.createWebMessageChannel();
                    closeExporter();
                    exporter = new FileExporter(MainActivity.this, ports[0]);
                    web.postWebMessage(new WebMessage("T2CAN_NATIVE_PORT", new WebMessagePort[]{ports[1]}),
                            Uri.parse(UrlPolicy.origin(dashboardHome)));
                    main.removeCallbacks(loadTimeout);
                    webResult = "main document loaded";
                    loaded = true;
                    dashboardLoadedOnce = true;
                    loadStarted = false;
                    connectionScreen.setVisibility(View.GONE);
                    menuButton.setVisibility(View.VISIBLE);
                    updateTheme();
                });
            }
            @Override public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                if (view == web && request.isForMainFrame()) pageFailed();
            }
            @Override public void onReceivedHttpError(WebView view, WebResourceRequest request, WebResourceResponse response) {
                if (view == web && request.isForMainFrame()) pageFailed();
            }
        });
        web.setWebChromeClient(new WebChromeClient() {
            @Override public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback, FileChooserParams params) {
                if(destroyed || view != web || !loaded) { callback.onReceiveValue(null); return true; }
                if (fileCallback != null) fileCallback.onReceiveValue(null);
                fileCallback = callback;
                if (!UrlPolicy.isDevice(view.getUrl(), dashboardHome)) { finishFileChoice(null); return true; }
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT)
                        .addCategory(Intent.CATEGORY_OPENABLE).setType("*/*")
                        .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                try { startActivityForResult(intent, FILE_REQUEST); }
                catch (Exception error) { finishFileChoice(null); toast(R.string.file_picker_unavailable); }
                return true;
            }
            @Override public boolean onJsAlert(WebView view, String url, String message, JsResult result) {
                if(destroyed || view != web || !UrlPolicy.isDevice(url,dashboardHome)) { result.cancel(); return true; }
                new AlertDialog.Builder(MainActivity.this).setTitle(R.string.app_name).setMessage(message)
                        .setPositiveButton(R.string.dialog_ok, (d, which) -> result.confirm())
                        .setOnCancelListener(d -> result.cancel()).show();
                return true;
            }
            @Override public boolean onJsConfirm(WebView view, String url, String message, JsResult result) {
                if(destroyed || view != web || !UrlPolicy.isDevice(url,dashboardHome)) { result.cancel(); return true; }
                new AlertDialog.Builder(MainActivity.this).setTitle(R.string.app_name).setMessage(message)
                        .setPositiveButton(R.string.dialog_ok, (d, which) -> result.confirm())
                        .setNegativeButton(R.string.dialog_cancel, (d, which) -> result.cancel())
                        .setOnCancelListener(d -> result.cancel()).show();
                return true;
            }
        });
        web.setDownloadListener((url, userAgent, disposition, mime, length) ->
                toast(R.string.use_dashboard_download));
    }

    private void showConnectionMenu() {
        if(fileCallback!=null) { toast(R.string.file_transfer_busy);return; }
        if(web==null) { panel.show();return; }
        final WebView current=web;final int epoch=documentEpoch;
        current.evaluateJavascript("window.__tuNativeBusy ? window.__tuNativeBusy() : true", result->{
            if(destroyed || current!=web || epoch!=documentEpoch) return;
            if(!"false".equals(result)) { toast(R.string.file_transfer_busy);return; }
            panel.show();
        });
    }

    private void pageFailed() {
        webResult="native health succeeded; WebView main request failed or timed out";
        disconnectDevice();
        connectionStatus.setText("Dashboard failed");
        connectionMessage.setText("Wi-Fi responded, but the dashboard could not load. Check the address, WebView, VPN or Private DNS settings, then retry.");
    }

    private void refreshProfile() {
        DeviceProfile p=profiles.selected();
        connectionLabel.setText(p.name);
        connectionFooter.setText(p.url+" · App 1.2.1");
        connectionMessage.setText(p.ssid.isEmpty()?"Register your device Wi-Fi in Devices, or use Manual connection.":"Ready to connect to "+p.ssid+".");
    }

    private void startConnection(boolean automatic) {
        launchConnection.consume();
        if(manager==null || manager.busy()) return;
        DeviceProfile profile=profiles.selected();
        String error=profile.validationError(automatic);
        if(error!=null) { toast(error);panel.edit(profile);return; }
        if(automatic && Build.VERSION.SDK_INT<29) { panel.manual();return; }
        String permission=automatic?PermissionPolicy.requiredPermission(Build.VERSION.SDK_INT,getApplicationInfo().targetSdkVersion):null;
        if(permission!=null && checkSelfPermission(permission)!=android.content.pm.PackageManager.PERMISSION_GRANTED) {
            pendingPermission=profile;
            connectionStatus.setText("Permission required");
            connectionMessage.setText("Allow nearby Wi-Fi access to connect. On Android 10–12, Android requires location permission for this Wi-Fi API. You can also use Manual connection.");
            requestPermissions(PermissionPolicy.requestPermissions(Build.VERSION.SDK_INT,getApplicationInfo().targetSdkVersion),WIFI_PERMISSION);return;
        }
        pendingPermission=null;
        manager.connect(profile,automatic);
    }

    @Override public void onRequestPermissionsResult(int request,String[] permissions,int[] results) {
        super.onRequestPermissionsResult(request,permissions,results);
        if(request!=WIFI_PERMISSION || pendingPermission==null || destroyed) return;
        DeviceProfile requested=pendingPermission;pendingPermission=null;
        if(!requested.id.equals(profiles.selected().id)) return;
        String needed=PermissionPolicy.requiredPermission(Build.VERSION.SDK_INT,getApplicationInfo().targetSdkVersion);
        if(needed!=null && checkSelfPermission(needed)==android.content.pm.PackageManager.PERMISSION_GRANTED) startConnection(true);
        else {
            connectionStatus.setText("Permission required");
            connectionMessage.setText("Wi-Fi permission was not granted. Use App permissions in connection settings, or Manual connection.");
        }
    }

    private void disconnectDevice() {
        launchConnection.consume();
        pendingPermission=null;
        if(manager!=null) manager.disconnect();
        destroyDashboard();
    }

    private void openDashboard(DeviceProfile profile) {
        if(destroyed) return;
        destroyDashboard();
        dashboardHome=profile.url;
        // Network health and verified process binding precede WebView construction.
        web=new WebView(this);
        FrameLayout.LayoutParams layout=new FrameLayout.LayoutParams(-1,-1);layout.bottomMargin=dp(40);
        root.addView(web,0,layout);
        configureWebView();updateTheme();
        webResult="loading";loadStarted=true;
        connectionMessage.setText("Wi-Fi ready. Opening the dashboard…");
        main.postDelayed(loadTimeout,20000);
        web.loadUrl(dashboardHome);
    }

    private void destroyDashboard() {
        main.removeCallbacks(loadTimeout);documentEpoch++;
        loaded=false;loadStarted=false;dashboardLoadedOnce=false;
        finishFileChoice(null);closeExporter();
        if(web!=null) {
            WebView old=web;web=null;
            old.stopLoading();root.removeView(old);old.destroy();
        }
        if(connectionScreen!=null) connectionScreen.setVisibility(View.VISIBLE);
        if(menuButton!=null) menuButton.setVisibility(View.GONE);
    }

    private void dashboardBack() {
        if (web == null || !loaded || connectionScreen.getVisibility() == View.VISIBLE) { finish(); return; }
        web.evaluateJavascript("window.__tuNativeBack ? window.__tuNativeBack() : 'exit'", value -> {
            if ("\"busy\"".equals(value)) toast(R.string.file_transfer_busy);
            else if ("\"modal\"".equals(value)) toast(R.string.modal_open);
            else if (!"\"handled\"".equals(value)) finish();
        });
    }

    @Override public void onBackPressed() { dashboardBack(); }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        Uri uri = result == RESULT_OK && data != null ? data.getData() : null;
        if (request == FILE_REQUEST) finishFileChoice(uri == null ? null : new Uri[]{uri});
        if (request == FileExporter.SAVE_REQUEST && exporter != null) exporter.onDestination(uri);
    }

    private void finishFileChoice(Uri[] files) {
        if (fileCallback != null) fileCallback.onReceiveValue(files);
        fileCallback = null;
    }

    @Override protected void onResume() {
        super.onResume();
        if (web != null) { web.onResume(); updateTheme(); }
        if(manager!=null) {
            manager.resume();
            String permission=PermissionPolicy.requiredPermission(Build.VERSION.SDK_INT,getApplicationInfo().targetSdkVersion);
            boolean granted=permission==null || checkSelfPermission(permission)==android.content.pm.PackageManager.PERMISSION_GRANTED;
            LaunchConnectionPolicy.Action action=launchConnection.onForeground(profiles.selected(),Build.VERSION.SDK_INT,granted);
            if(action==LaunchConnectionPolicy.Action.CONNECT) startConnection(true);
            else if(action==LaunchConnectionPolicy.Action.NEEDS_PERMISSION) {
                connectionStatus.setText("Permission required");
                connectionMessage.setText("Tap Connect once to allow Wi-Fi access. After approval, this device will connect automatically when you reopen the app.");
            }
        }
    }

    @Override public Object onRetainNonConfigurationInstance() {
        return launchConnection;
    }

    @Override public void onConfigurationChanged(Configuration config) {
        super.onConfigurationChanged(config);
        if(root!=null) updateTheme();
    }

    private String readAsset(String path) {
        try (InputStream input = getAssets().open(path); ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int count;
            while ((count = input.read(buffer)) != -1) out.write(buffer, 0, count);
            return new String(out.toByteArray(), StandardCharsets.UTF_8);
        } catch (Exception error) { throw new IllegalStateException(error); }
    }

    private void closeExporter() { if (exporter != null) exporter.close(); exporter = null; }
    private void toast(String text) { Toast.makeText(this, text, Toast.LENGTH_LONG).show(); }
    private void toast(int resource) { toast(getString(resource)); }

    @Override protected void onDestroy() {
        destroyed = true;
        main.removeCallbacksAndMessages(null);
        if(manager!=null) manager.close();
        finishFileChoice(null);
        closeExporter();
        destroyDashboard();
        super.onDestroy();
    }
}
