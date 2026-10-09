package dev.t2can.unlock;
public final class PermissionPolicyTest {
 public static void main(String[] args) {
  int[][] cases={{26,36},{28,36},{29,28},{29,36},{30,36},{31,36},{32,36},{33,32},{33,33},{34,36},{36,36},{37,36}};
  String location="android.permission.ACCESS_FINE_LOCATION", nearby="android.permission.NEARBY_WIFI_DEVICES";
  String[] expected={null,null,location,location,location,location,location,location,nearby,nearby,nearby,nearby};
  for(int i=0;i<cases.length;i++) {
   String actual=PermissionPolicy.requiredPermission(cases[i][0],cases[i][1]);
   if(!java.util.Objects.equals(actual,expected[i]))throw new AssertionError("SDK "+cases[i][0]+" target "+cases[i][1]);
  }
  if (!java.util.Arrays.equals(PermissionPolicy.requestPermissions(31,36),new String[]{location,"android.permission.ACCESS_COARSE_LOCATION"}))
   throw new AssertionError("Android12 must request precise and approximate location together");
  if(PermissionPolicy.requestPermissions(29,36).length!=1 || PermissionPolicy.requestPermissions(33,36).length!=1 || PermissionPolicy.requestPermissions(28,36).length!=0)
   throw new AssertionError("permission request scope");
  System.out.println("PermissionPolicyTest: "+(cases.length+2)+" checks PASS");
 }
}
