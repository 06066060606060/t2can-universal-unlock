package dev.t2can.unlock;

import android.content.Context;
import android.content.SharedPreferences;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.KeyStore;
import java.util.ArrayList;
import java.util.List;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;

/** Entire profile document is encrypted. Corruption fails closed without overwriting saved data. */
public final class ProfileRepository {
    private static final String ALIAS = "t2can-device-profiles-v1";
    private static final byte[] AAD = "dev.t2can.unlock/profiles/v1".getBytes(StandardCharsets.UTF_8);
    private final SharedPreferences preferences;
    private List<DeviceProfile> profiles = new ArrayList<>();
    private String selectedId;

    public ProfileRepository(Context context) throws Exception {
        preferences = context.getApplicationContext().getSharedPreferences("device_profiles", Context.MODE_PRIVATE);
        String stored = preferences.getString("encrypted_v1", null);
        if (stored == null) {
            profiles.add(DeviceProfile.legacy());
            selectedId = "legacy";
            return;
        }
        if (stored.length() > 262144) throw new IOException("Profile storage is invalid.");
        byte[] packed = Base64.decode(stored, Base64.NO_WRAP);
        if (packed.length < 29) throw new IOException("Profile storage is invalid.");
        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(Cipher.DECRYPT_MODE, key(false), new GCMParameterSpec(128, packed, 0, 12));
        cipher.updateAAD(AAD);
        JSONObject document = new JSONObject(new String(cipher.doFinal(packed, 12, packed.length - 12), StandardCharsets.UTF_8));
        if (document.getInt("version") != 1) throw new IOException("Unsupported profile storage version.");
        JSONArray items = document.getJSONArray("profiles");
        if (items.length() < 1 || items.length() > 20) throw new IOException("Profile storage is invalid.");
        for (int i = 0; i < items.length(); i++) {
            JSONObject item = items.getJSONObject(i);
            DeviceProfile profile = new DeviceProfile(item.getString("id"), item.getString("name"), item.getString("ssid"),
                    item.getString("security"), item.getString("password"), item.getString("url"));
            if (profile.validationError(false) != null || find(profile.id) != null)
                throw new IOException("Profile storage is invalid.");
            profiles.add(profile);
        }
        selectedId = document.getString("selected");
        if (find(selectedId) == null) throw new IOException("Selected profile is missing.");
    }
    public synchronized List<DeviceProfile> all() { return new ArrayList<>(profiles); }
    public synchronized DeviceProfile selected() { return find(selectedId); }
    public synchronized void save(DeviceProfile profile) throws Exception {
        String error = profile.validationError(false);
        if (error != null) throw new IllegalArgumentException(error);
        List<DeviceProfile> next = new ArrayList<>(profiles);
        DeviceProfile previous = find(profile.id);
        if (previous != null) next.set(next.indexOf(previous), profile);
        else {
            if (next.size() >= 20) throw new IllegalArgumentException("Keep at most 20 device profiles.");
            next.add(profile);
        }
        persist(next, selectedId);
    }
    public synchronized void select(String id) throws Exception {
        if (find(id) == null) throw new IllegalArgumentException("Device profile not found.");
        persist(profiles, id);
    }
    public synchronized void delete(String id) throws Exception {
        DeviceProfile removed = find(id);
        if (removed == null) return;
        List<DeviceProfile> next = new ArrayList<>(profiles);
        next.remove(removed);
        if (next.isEmpty()) next.add(DeviceProfile.legacy());
        persist(next, id.equals(selectedId) ? next.get(0).id : selectedId);
    }
    private DeviceProfile find(String id) {
        for (DeviceProfile profile : profiles) if (profile.id.equals(id)) return profile;
        return null;
    }
    private SecretKey key(boolean create) throws Exception {
        KeyStore store = KeyStore.getInstance("AndroidKeyStore");
        store.load(null);
        if (!store.containsAlias(ALIAS)) {
            if (!create) throw new IOException("Profile encryption key is unavailable.");
            KeyGenerator generator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore");
            generator.init(new KeyGenParameterSpec.Builder(ALIAS, KeyProperties.PURPOSE_ENCRYPT | KeyProperties.PURPOSE_DECRYPT)
                    .setBlockModes(KeyProperties.BLOCK_MODE_GCM).setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                    .setKeySize(256).build());
            return generator.generateKey();
        }
        return (SecretKey) store.getKey(ALIAS, null);
    }
    private void persist(List<DeviceProfile> next, String selection) throws Exception {
        JSONArray items = new JSONArray();
        for (DeviceProfile profile : next) items.put(new JSONObject().put("id", profile.id).put("name", profile.name)
                .put("ssid", profile.ssid).put("security", profile.security).put("password", profile.password).put("url", profile.url));
        byte[] plain = new JSONObject().put("version", 1).put("selected", selection).put("profiles", items)
                .toString().getBytes(StandardCharsets.UTF_8);
        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, key(true));
        cipher.updateAAD(AAD);
        byte[] encrypted = cipher.doFinal(plain), iv = cipher.getIV();
        if (iv.length != 12) throw new IOException("Profile encryption failed.");
        byte[] packed = new byte[iv.length + encrypted.length];
        System.arraycopy(iv, 0, packed, 0, iv.length);
        System.arraycopy(encrypted, 0, packed, iv.length, encrypted.length);
        if (!preferences.edit().putString("encrypted_v1", Base64.encodeToString(packed, Base64.NO_WRAP)).commit())
            throw new IOException("Could not save device profiles.");
        profiles = new ArrayList<>(next);
        selectedId = selection;
    }
}
