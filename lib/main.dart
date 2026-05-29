import 'package:flutter/material.dart';
import 'iot_service.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'IoT Lightbulb Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.amber),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'AWS IoT Lightbulb'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  bool _isLightOn = false;
  final AwsIotService _iotService = AwsIotService();

  @override
  void initState() {
    super.initState();
    _initIot();
  }

  Future<void> _initIot() async {
    await _iotService.connect();
    // Force a rebuild to update connection status if needed
    if (mounted) setState(() {});
  }

  void _toggleLight() {
    setState(() {
      _isLightOn = !_isLightOn;
    });
    _iotService.publishState(_isLightOn);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.lightbulb,
              size: 150,
              color: _isLightOn ? Colors.amber : Colors.grey,
            ),
            const SizedBox(height: 20),
            Text(
              'Light is ${_isLightOn ? "ON" : "OFF"}',
              style: Theme.of(context).textTheme.headlineMedium,
            ),
            const SizedBox(height: 40),
            ElevatedButton(
              onPressed: _toggleLight,
              child: Text(_isLightOn ? 'Switch OFF' : 'Switch ON'),
            ),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    _iotService.disconnect();
    super.dispose();
  }
}
