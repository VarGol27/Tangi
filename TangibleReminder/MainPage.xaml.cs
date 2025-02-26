using Plugin.BLE;
using Plugin.BLE.Abstractions.Contracts;
using System;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Maui.Controls;
using Microsoft.Maui.ApplicationModel;

namespace TangibleReminder
{
    public partial class MainPage : ContentPage
    {
        private IAdapter adapter;
        private IDevice esp32Device;

        private readonly Guid serviceUuid = Guid.Parse("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
        private readonly Guid characteristicUuid = Guid.Parse("beb5483e-36e1-4688-b7f5-ea07361b26a8");

        public MainPage()
        {
            InitializeComponent();

            // Display current date and time
            currentTimeLabel.Text = "Current Time: " + DateTime.Now.ToString("yyyy-MM-dd HH:mm");

            try
            {
                adapter = CrossBluetoothLE.Current.Adapter;
                if (adapter == null)
                {
                    statusLabel.Text = "Bluetooth adapter initialization failed.";
                }
            }
            catch (Exception ex)
            {
                statusLabel.Text = "Error initializing adapter.";
            }
        }


        private async void OnSetTimerClicked(object sender, EventArgs e)
        {
            statusLabel.Text = "Requesting permissions...";

#if ANDROID
            await RequestBluetoothPermissionsAsync();
#endif

            statusLabel.Text = "Connecting...";

            try
            {
                string reminderName = reminderNameEntry.Text?.Trim();

                if (string.IsNullOrEmpty(reminderName))
                {
                    statusLabel.Text = "Please enter a reminder name.";
                    return;
                }

                // Find and connect to the ESP32 device
                await FindAndConnectToEsp32();
                if (esp32Device == null)
                {
                    statusLabel.Text = "Reminder not found.";
                    return;
                }

                DateTime targetTime = reminderDate.Date.Add(reminderTime.Time);

                DateTime currentTime = DateTime.Now;

                // Send both the current time and the target time
                await SendTimesToEsp32(currentTime, targetTime);

                reminderNameLabel.Text = $"Reminder: {reminderName}\nTime: {targetTime:yyyy-MM-dd HH:mm}";
                statusLabel.Text = "Reminder set!";
            }
            catch (Exception ex)
            {
                statusLabel.Text = "Error: " + ex.Message;
            }
        }


#if ANDROID
        private async Task RequestBluetoothPermissionsAsync()
        {
            try
            {
                // Check and request Location permission for BLE scanning
                var statusLocation = await Permissions.CheckStatusAsync<Permissions.LocationWhenInUse>();
                if (statusLocation != PermissionStatus.Granted)
                {
                    statusLocation = await Permissions.RequestAsync<Permissions.LocationWhenInUse>();
                }

                if (DeviceInfo.Platform == DevicePlatform.Android && DeviceInfo.Version.Major >= 12)
                {
                    var statusBluetoothScan = await Permissions.CheckStatusAsync<BluetoothPermissions.BluetoothScan>();
                    if (statusBluetoothScan != PermissionStatus.Granted)
                    {
                        await Permissions.RequestAsync<BluetoothPermissions.BluetoothScan>();
                    }

                    var statusBluetoothConnect = await Permissions.CheckStatusAsync<BluetoothPermissions.BluetoothConnect>();
                    if (statusBluetoothConnect != PermissionStatus.Granted)
                    {
                        await Permissions.RequestAsync<BluetoothPermissions.BluetoothConnect>();
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Permission error: {ex.Message}");
            }
        }
#endif

        private async Task FindAndConnectToEsp32()
        {
            esp32Device = null;
            Console.WriteLine("Starting BLE scan...");

            adapter.DeviceDiscovered += (s, a) =>
            {
                if (a.Device.Name == "ESP32_Feather_BLE")
                {
                    esp32Device = a.Device;
                    _ = adapter.StopScanningForDevicesAsync();
                }
            };

            await adapter.StartScanningForDevicesAsync();
            await Task.Delay(30000);
            await adapter.StopScanningForDevicesAsync();

            if (esp32Device == null)
            {
                statusLabel.Text = "Device not found.";
                return;
            }

            await adapter.ConnectToDeviceAsync(esp32Device);
        }


        private async Task SendTimesToEsp32(DateTime currentTime, DateTime targetTime)
        {
            if (esp32Device == null)
            {
                statusLabel.Text = "Reminder is not connected.";
                return;
            }

            var service = await esp32Device.GetServiceAsync(serviceUuid);
            if (service == null)
            {
                statusLabel.Text = "Service not found";
                return;
            }

            var characteristic = await service.GetCharacteristicAsync(characteristicUuid);
            if (characteristic == null)
            {
                statusLabel.Text = "Characteristic not found";
                return;
            }

            string currentTimeFormatted = currentTime.ToString("yyyy-MM-dd HH:mm");
            await characteristic.WriteAsync(Encoding.UTF8.GetBytes("settime " + currentTimeFormatted));

            string targetTimeFormatted = targetTime.ToString("yyyy-MM-dd HH:mm");
            await characteristic.WriteAsync(Encoding.UTF8.GetBytes("targettime " + targetTimeFormatted));

            statusLabel.Text = "Reminder set!";
        }



    }
}