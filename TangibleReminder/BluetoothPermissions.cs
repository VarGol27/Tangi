using Microsoft.Maui.ApplicationModel;

namespace TangibleReminder
{
#if ANDROID
    public class BluetoothPermissions
    {
        public class BluetoothScan : Permissions.BasePlatformPermission
        {
            public override (string androidPermission, bool isRuntime)[] RequiredPermissions =>
                new (string, bool)[] { (global::Android.Manifest.Permission.BluetoothScan, true) };
        }

        public class BluetoothConnect : Permissions.BasePlatformPermission
        {
            public override (string androidPermission, bool isRuntime)[] RequiredPermissions =>
                new (string, bool)[] { (global::Android.Manifest.Permission.BluetoothConnect, true) };
        }
    }
#endif
}
