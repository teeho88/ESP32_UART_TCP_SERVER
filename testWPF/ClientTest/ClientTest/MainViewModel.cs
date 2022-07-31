using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace ClientTest
{
    public class MainViewModel:ViewModelBase
    {
        TcpClient client = new TcpClient();
        public MainViewModel()
        {
            ConnectStatus = "Status: OK";
            ServerIP = "192.168.43.100";
            ServerPort = "3000";
            Respond = "";
            ConnectCM = new RelayCommand(x =>
            {
                Thread client_thread = new Thread(new ThreadStart(create_client_connect));
                client_thread.Start();
            });

            DisConnectCM = new RelayCommand(x =>
            {
                client.Close();
            });
        }

        void create_client_connect()
        {
            try
            {
                ConnectStatus = "Connect......";
                // 1. connect
                client.Connect(IPAddress.Parse(ServerIP), int.Parse(ServerPort));
                // 2. tao stream reader doc du lieu
                Stream stream = client.GetStream();
                var reader = new StreamReader(stream);
                ConnectStatus = "Connected to Server";
                while (true)
                {
                    // 3. receive
                    string str = reader.ReadLine();
                    Respond += str;
                    if (str == null)
                    {
                        break;
                    }
                    if (str.Contains("end"))
                    {
                        break;
                    }
                }

                // 4. close connection
                stream.Close();
                client.Close();
                ConnectStatus = "Disconnected";
            }
            catch (Exception ex)
            {
                ConnectStatus = "Error: " + ex;
                client.Close();
            }
        }

        public string ServerIP { get; set; } //property dung de trao doi du lieu voi giao dien (UI)
        public string ServerPort { get; set; }
        public RelayCommand ConnectCM { get; set; } //command de tao hanh dong khi nhan nut
        public RelayCommand DisConnectCM { get; set; }

        private string respond;
        public string Respond { get => respond; set => SetProperty(ref respond, value); } //property dung de lien ket voi giao dien (UI)

        private string connectStatus;
        public string ConnectStatus { get => connectStatus; set => SetProperty(ref connectStatus, value); }
    }
}
