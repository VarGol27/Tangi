using System.Threading.Tasks;

namespace TangibleReminder
{
    public interface IBLEService
    {
        Task ConnectAsync(string deviceName);
        Task SendDataAsync(string data);
        Task DisconnectAsync();
    }
}
