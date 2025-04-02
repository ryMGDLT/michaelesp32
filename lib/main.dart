import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

void main() {
  runApp(const RelayControlApp());
}

class RelayControlApp extends StatelessWidget {
  const RelayControlApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Relay Control',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        useMaterial3: true,
      ),
      debugShowCheckedModeBanner: false,
      home: const RelayControlPage(),
    );
  }
}

class RelayControlPage extends StatefulWidget {
  const RelayControlPage({super.key});

  @override
  State<RelayControlPage> createState() => _RelayControlPageState();
}

class _RelayControlPageState extends State<RelayControlPage> {
  String esp32Ip = "192.168.8.116"; 
  String ssid = "Loading...";
  
  List<bool> relayStates = [false, false, false, false];
  
  List<TimeOfDay> onTimes = [
    const TimeOfDay(hour: 8, minute: 0),
    const TimeOfDay(hour: 9, minute: 0),
    const TimeOfDay(hour: 10, minute: 0),
    const TimeOfDay(hour: 11, minute: 0),
  ];
  List<TimeOfDay> offTimes = [
    const TimeOfDay(hour: 18, minute: 0),
    const TimeOfDay(hour: 19, minute: 0),
    const TimeOfDay(hour: 20, minute: 0),
    const TimeOfDay(hour: 21, minute: 0),
  ];

  @override
  void initState() {
    super.initState();
    _fetchInitialData();
  }

  Future<void> _fetchInitialData() async {
    try {
      final response = await http.get(Uri.parse('http://$esp32Ip/'));
      if (response.statusCode == 200) {
        setState(() {
          ssid = _extractSSID(response.body);
        });
      }
    } catch (e) {
      setState(() {
        ssid = "Error connecting";
      });
    }
  }

  String _extractSSID(String html) {
    final ssidStart = html.indexOf('SSID: ') + 6;
    final ssidEnd = html.indexOf('</p>', ssidStart);
    return html.substring(ssidStart, ssidEnd);
  }

  Future<void> _selectTime(BuildContext context, int relayIndex, bool isOnTime) async {
    final TimeOfDay? picked = await showTimePicker(
      context: context,
      initialTime: isOnTime ? onTimes[relayIndex] : offTimes[relayIndex],
    );
    if (picked != null) {
      setState(() {
        if (isOnTime) {
          onTimes[relayIndex] = picked;
        } else {
          offTimes[relayIndex] = picked;
        }
      });
    }
  }

  Future<void> _sendSettingsToESP() async {
    try {
      final Map<String, String> data = {
        'onTime1': _formatTime(onTimes[0]),
        'offTime1': _formatTime(offTimes[0]),
        'onTime2': _formatTime(onTimes[1]),
        'offTime2': _formatTime(offTimes[1]),
        'onTime3': _formatTime(onTimes[2]),
        'offTime3': _formatTime(offTimes[2]),
        'onTime4': _formatTime(onTimes[3]),
        'offTime4': _formatTime(offTimes[3]),
      };

      final response = await http.post(
        Uri.parse('http://$esp32Ip/set'),
        body: data,
      );

      if (response.statusCode == 200) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Settings saved successfully!')),
        );
      } else {
        throw Exception('Failed to save settings: ${response.statusCode}');
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error: $e')),
      );
    }
  }

  Future<void> _toggleRelay(int index, bool value) async {
    try {
      final response = await http.post(
        Uri.parse('http://$esp32Ip/toggle'),
        body: {
          'relay': (index + 1).toString(),
          'state': value ? '1' : '0',
        },
      );

      if (response.statusCode == 200) {
        setState(() {
          relayStates[index] = value;
        });
      } else {
        throw Exception('Failed to toggle relay: ${response.statusCode}');
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error toggling relay: $e')),
      );
    }
  }

  Future<void> _resetWiFi() async {
    try {
      final response = await http.get(Uri.parse('http://$esp32Ip/reset'));
      if (response.statusCode == 200) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('WiFi reset requested. ESP32 will restart.')),
        );
        setState(() {
          ssid = "Resetting...";
        });
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error resetting WiFi: $e')),
      );
    }
  }

  String _formatTime(TimeOfDay time) {
    return '${time.hour.toString().padLeft(2, '0')}:${time.minute.toString().padLeft(2, '0')}';
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Activity 4 RTOS ESP32'),
        centerTitle: true,
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            const Text(
              'Set Relay Times',
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.grey),
            ),
            const SizedBox(height: 20),
            ...List.generate(4, (index) => _buildRelayControl(index)),
            const SizedBox(height: 20),
            ElevatedButton(
              onPressed: _sendSettingsToESP,
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 15),
                backgroundColor: const Color(0xFF4CAF50),
              ),
              child: const Text(
                'Set All Times',
                style: TextStyle(fontSize: 16, color: Colors.white),
              ),
            ),
            const SizedBox(height: 30),
            Container(
              padding: const EdgeInsets.all(15),
              decoration: BoxDecoration(
                color: const Color(0xFFF0F0F0),
                borderRadius: BorderRadius.circular(5),
              ),
              child: Column(
                children: [
                  Text('Current IP: $esp32Ip'),
                  Text('SSID: $ssid'),
                  TextButton(
                    onPressed: _resetWiFi,
                    style: TextButton.styleFrom(
                      foregroundColor: const Color(0xFF0066CC),
                    ),
                    child: const Text('Reset WiFi Settings'),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildRelayControl(int index) {
    return Card(
      margin: const EdgeInsets.only(bottom: 10),
      child: Padding(
        padding: const EdgeInsets.all(15.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Relay ${index + 1}',
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 10),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text('ON Time (HH:MM):'),
                TextButton(
                  onPressed: () => _selectTime(context, index, true),
                  child: Text(_formatTime(onTimes[index])),
                ),
              ],
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text('OFF Time (HH:MM):'),
                TextButton(
                  onPressed: () => _selectTime(context, index, false),
                  child: Text(_formatTime(offTimes[index])),
                ),
              ],
            ),
            const SizedBox(height: 10),
            SwitchListTile(
              title: Text('Relay ${index + 1} State'),
              value: relayStates[index],
              onChanged: (value) => _toggleRelay(index, value),
              activeColor: const Color(0xFF4CAF50),
            ),
          ],
        ),
      ),
    );
  }
}