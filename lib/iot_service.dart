import 'dart:convert';
import 'dart:io';
import 'package:flutter/services.dart' show rootBundle;
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class AwsIotService {
  // IMPORTANT: Replace with your actual AWS IoT endpoint
  final String _endpoint = 'a3r361v405h13g-ats.iot.ap-south-1.amazonaws.com';
  final String _clientId = 'phone_controller';
  final String _topic = 'sdk/test/java';

  late MqttServerClient client;

  Future<void> connect() async {
    client = MqttServerClient.withPort(_endpoint, _clientId, 8883);
    client.secure = true;
    client.logging(on: true);
    client.setProtocolV311();
    client.keepAlivePeriod = 20;

    try {
      // Load Certificates from Assets
      // Ensure these files exist in assets/certs/
      final rootCA = await rootBundle.load('assets/certs/AmazonRootCA1.pem');
      final deviceCert = await rootBundle.load('assets/certs/certificate.pem.crt');
      final privateKey = await rootBundle.load('assets/certs/private.pem.key');

      final SecurityContext context = SecurityContext.defaultContext;
      context.setClientAuthoritiesBytes(rootCA.buffer.asUint8List());
      context.useCertificateChainBytes(deviceCert.buffer.asUint8List());
      context.usePrivateKeyBytes(privateKey.buffer.asUint8List());

      client.securityContext = context;

      final connMessage = MqttConnectMessage()
          .withClientIdentifier(_clientId)
          .startClean();
      client.connectionMessage = connMessage;

      print('AwsIotService: Connecting to AWS IoT...');
      await client.connect();
    } catch (e) {
      print('AwsIotService Exception: $e');
      client.disconnect();
    }

    if (client.connectionStatus!.state == MqttConnectionState.connected) {
      print('AwsIotService: Connected to AWS IoT Core');
    } else {
      print('AwsIotService: Connection failed - status is ${client.connectionStatus}');
      client.disconnect();
    }
  }

  void publishState(bool isOn) {
    if (client.connectionStatus?.state != MqttConnectionState.connected) {
      print('AwsIotService: Cannot publish, not connected');
      return;
    }

    final String payload = jsonEncode({
      'state': isOn ? 'ON' : 'OFF',
      'timestamp': DateTime.now().toIso8601String(),
    });

    final builder = MqttClientPayloadBuilder();
    builder.addString(payload);

    print('AwsIotService: Publishing state $payload to $_topic');
    client.publishMessage(_topic, MqttQos.atLeastOnce, builder.payload!);
  }

  void disconnect() {
    client.disconnect();
  }
}
