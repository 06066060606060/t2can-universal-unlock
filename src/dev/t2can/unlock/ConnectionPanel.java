package dev.t2can.unlock;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.net.Uri;
import android.provider.Settings;
import android.text.InputType;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.TextView;
import android.widget.Toast;
import java.util.List;
import java.util.UUID;

/** Native controls only; dashboard documents and credentials never cross a JS bridge. */
final class ConnectionPanel {
    interface Actions {
        void connect(boolean automatic);
        void disconnect();
        void profileChanged();
        String diagnostics();
    }
    private final Activity activity;
    private final ProfileRepository profiles;
    private final Actions actions;
    ConnectionPanel(Activity activity, ProfileRepository profiles, Actions actions) {
        this.activity=activity; this.profiles=profiles; this.actions=actions;
    }
    void show() {
        new AlertDialog.Builder(activity).setTitle("T2CAN connection")
            .setItems(new String[]{"Connect", "Devices", "Manual connection", "Disconnect", "Copy diagnostics", "Wi-Fi settings", "App permissions"}, (d,which) -> {
                switch(which) {
                    case 0: actions.connect(true); break;
                    case 1: devices(); break;
                    case 2: manual(); break;
                    case 3: actions.disconnect(); break;
                    case 4:
                        ((ClipboardManager)activity.getSystemService(Activity.CLIPBOARD_SERVICE))
                            .setPrimaryClip(ClipData.newPlainText("T2CAN diagnostics",actions.diagnostics()));
                        toast("Diagnostics copied (no credentials)."); break;
                    case 5: wifi(); break;
                    case 6: activity.startActivity(new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                            Uri.parse("package:"+activity.getPackageName()))); break;
                }
            }).setNegativeButton("Close",null).show();
    }
    void wifi() { activity.startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS)); }
    void manual() {
        new AlertDialog.Builder(activity).setTitle("Manual connection")
            .setMessage("Connect to your T2CAN in Android Wi-Fi settings, then return and tap Check connection. Other apps use the system default internet route; cellular internet is not guaranteed in this mode.")
            .setPositiveButton("Check connection",(d,w)->actions.connect(false))
            .setNeutralButton("Wi-Fi settings",(d,w)->wifi()).setNegativeButton("Cancel",null).show();
    }
    void devices() {
        List<DeviceProfile> list=profiles.all(); String[] labels=new String[list.size()+1];
        for(int i=0;i<list.size();i++) labels[i]=list.get(i).name+(list.get(i).id.equals(profiles.selected().id)?" (selected)":"");
        labels[list.size()]="Add device";
        new AlertDialog.Builder(activity).setTitle("Devices").setItems(labels,(d,w)->{
            if(w==list.size()) { edit(null); return; }
            DeviceProfile profile=list.get(w);
            new AlertDialog.Builder(activity).setTitle(profile.name).setItems(new String[]{"Select", "Edit", "Delete"},(dialog,choice)->{
                try {
                    if(choice==0) { actions.disconnect(); profiles.select(profile.id); actions.profileChanged(); }
                    else if(choice==1) edit(profile);
                    else new AlertDialog.Builder(activity).setTitle("Delete device?")
                        .setMessage("Only this app's saved connection profile will be removed.")
                        .setPositiveButton("Delete",(x,y)->{
                            try { actions.disconnect(); profiles.delete(profile.id); actions.profileChanged(); }
                            catch(Exception error) { storageError(); }
                        }).setNegativeButton("Cancel",null).show();
                } catch(Exception error) { storageError(); }
            }).setNegativeButton("Cancel",null).show();
        }).setNegativeButton("Close",null).show();
    }
    private EditText field(LinearLayout form,String label,String value,boolean password) {
        TextView title=new TextView(activity);title.setText(label);form.addView(title);
        EditText input=new EditText(activity);input.setSingleLine(true);
        input.setInputType(InputType.TYPE_CLASS_TEXT | (password ? InputType.TYPE_TEXT_VARIATION_PASSWORD : InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS));
        input.setText(value); form.addView(input,new LinearLayout.LayoutParams(-1,-2));return input;
    }
    void edit(DeviceProfile old) {
        LinearLayout form=new LinearLayout(activity);form.setOrientation(LinearLayout.VERTICAL);
        int pad=Math.round(20*activity.getResources().getDisplayMetrics().density);form.setPadding(pad,pad,pad,pad);
        EditText name=field(form,"Device name",old==null?"T2CAN":old.name,false);
        EditText ssid=field(form,"Exact Wi-Fi name (SSID)",old==null?"":old.ssid,false);
        TextView securityLabel=new TextView(activity);securityLabel.setText("Wi-Fi security");form.addView(securityLabel);
        Spinner security=new Spinner(activity);String[] types={"OPEN","WPA2","WPA3"};
        security.setAdapter(new ArrayAdapter<String>(activity,android.R.layout.simple_spinner_dropdown_item,types));
        security.setSelection(old==null?1:java.util.Arrays.asList(types).indexOf(old.security));form.addView(security);
        EditText password=field(form,"Wi-Fi password",old==null?"":old.password,true);
        EditText url=field(form,"Dashboard address",old==null?UrlPolicy.HOME:old.url,false);
        TextView hint=new TextView(activity);hint.setText("Use the exact settings of your device. HTTP is allowed only for 192.168.4.1; other private IPv4 addresses require HTTPS with a trusted certificate. Leave SSID blank only for manual mode.");form.addView(hint);
        TextView error=new TextView(activity);form.addView(error);
        ScrollView scroll=new ScrollView(activity);scroll.addView(form);
        AlertDialog dialog=new AlertDialog.Builder(activity).setTitle(old==null?"Add device":"Edit device")
            .setView(scroll).setPositiveButton("Save",null).setNegativeButton("Cancel",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            String home=url.getText().toString().trim(); if(!home.endsWith("/")) home+="/";
            DeviceProfile profile=new DeviceProfile(old==null?UUID.randomUUID().toString():old.id,
                name.getText().toString().trim(),ssid.getText().toString(),types[security.getSelectedItemPosition()],password.getText().toString(),home);
            String problem=profile.validationError(!profile.ssid.isEmpty());
            if(problem!=null) { error.setText(problem);return; }
            try { actions.disconnect();profiles.save(profile);profiles.select(profile.id);actions.profileChanged();dialog.dismiss(); }
            catch(Exception failed) { error.setText("Could not securely save profile. Existing profiles were kept."); }
        }));dialog.show();
        // Do not offer credentials to keyboard learning or autofill services.
        password.setImportantForAutofill(android.view.View.IMPORTANT_FOR_AUTOFILL_NO);
    }
    private void toast(String s) { Toast.makeText(activity,s,Toast.LENGTH_LONG).show(); }
    private void storageError() { toast("Could not update secure profiles. Existing data was kept."); }
}
