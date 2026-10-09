package dev.t2can.unlock;

/** One foreground launch decision; reusable across configuration-only recreation. */
public final class LaunchConnectionPolicy {
    public enum Action { NONE, CONNECT, NEEDS_PERMISSION }
    private boolean handled;
    public Action onForeground(DeviceProfile selected, int sdk, boolean permissionGranted) {
        if (handled) return Action.NONE;
        handled = true;
        if (sdk < 29 || selected == null || selected.validationError(true) != null) return Action.NONE;
        return permissionGranted ? Action.CONNECT : Action.NEEDS_PERMISSION;
    }
    public void consume() { handled = true; }
}
