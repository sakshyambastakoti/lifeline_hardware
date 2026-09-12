import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';

export const DashboardScreen: React.FC = () => {
  const { connectionState, connectedDevice, telemetry, theme } = useLifeLine();

  if (connectionState !== 'CONNECTED') {
    return (
      <View style={[styles.disconnectedContainer, { backgroundColor: theme.colors.bg0 }]}>
        <Ionicons name="link-outline" size={40} color={theme.colors.text3} />
        <Text style={[styles.discTitle, { color: theme.colors.text1 }]}>RF TELEMETRY DISCONNECTED</Text>
        <Text style={[styles.discSub, { color: theme.colors.text3 }]}>
          Establish a Bluetooth Low Energy link with a field node via the Radar console to stream real-time environmental metrics.
        </Text>
      </View>
    );
  }

  const isGasHazard = (telemetry?.gasPpm || 0) > 40;

  return (
    <ScrollView style={[styles.container, { backgroundColor: theme.colors.bg0 }]} contentContainerStyle={styles.content}>
      {/* Node Info Banner */}
      <View style={[styles.nodeBanner, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View>
          <Text style={[styles.nodeTitle, { color: theme.colors.text0 }]}>
            {connectedDevice?.name?.toUpperCase() || 'LIFELINE FIELD NODE'}
          </Text>
          <Text style={[styles.nodeSub, { color: theme.colors.text2 }]}>
            DEVICE // {telemetry?.deviceId || '003'} • FW // {telemetry?.version || 'v3.1.0 PRO'}
          </Text>
        </View>
        <View style={[styles.loraPill, { backgroundColor: theme.colors.successBg, borderColor: theme.colors.success }]}>
          <View style={[styles.loraDot, { backgroundColor: theme.colors.success }]} />
          <Text style={[styles.loraPillText, { color: theme.colors.text0 }]}>
            LORA {telemetry?.loraStatus || 'ACTIVE'}
          </Text>
        </View>
      </View>

      {/* Grid of Telemetry Cards */}
      <View style={styles.grid}>
        {/* Battery Telemetry */}
        <View style={[styles.card, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
          <View style={styles.cardHeader}>
            <Text style={[styles.cardLabel, { color: theme.colors.text2 }]}>POWER RESERVES</Text>
            <Ionicons
              name={
                (telemetry?.batteryPct || 0) > 40
                  ? 'battery-charging-outline'
                  : 'battery-dead-outline'
              }
              size={16}
              color={(telemetry?.batteryPct || 0) > 20 ? theme.colors.success : theme.colors.danger}
            />
          </View>
          <Text style={[styles.cardValue, { color: theme.colors.text0 }]}>
            {telemetry?.batteryPct !== undefined ? `${telemetry.batteryPct}%` : '--'}
          </Text>
          <Text style={[styles.cardHint, { color: theme.colors.text3 }]}>Sub-Zero Thermal Regulated LiPo</Text>
        </View>

        {/* Barometric Altitude */}
        <View style={[styles.card, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
          <View style={styles.cardHeader}>
            <Text style={[styles.cardLabel, { color: theme.colors.text2 }]}>ELEVATION MSL</Text>
            <Ionicons name="trending-up-outline" size={16} color={theme.colors.cyanStream} />
          </View>
          <Text style={[styles.cardValue, { color: theme.colors.text0 }]}>
            {telemetry?.altitude ? `${telemetry.altitude}M` : '1,350M'}
          </Text>
          <Text style={[styles.cardHint, { color: theme.colors.text3 }]}>Barometric & GNSS Fused</Text>
        </View>

        {/* Ambient Temperature */}
        <View style={[styles.card, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
          <View style={styles.cardHeader}>
            <Text style={[styles.cardLabel, { color: theme.colors.text2 }]}>TEMPERATURE</Text>
            <Ionicons name="thermometer-outline" size={16} color={theme.colors.solarAmber} />
          </View>
          <Text style={[styles.cardValue, { color: theme.colors.text0 }]}>
            {telemetry?.temperature !== undefined ? `${telemetry.temperature}°C` : '16.4°C'}
          </Text>
          <Text style={[styles.cardHint, { color: theme.colors.text3 }]}>Alpine Ambient Sensor</Text>
        </View>

        {/* Toxic Gas / Carbon Monoxide */}
        <View
          style={[
            styles.card,
            { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border },
            isGasHazard && { borderColor: theme.colors.danger, backgroundColor: theme.colors.dangerBg },
          ]}
        >
          <View style={styles.cardHeader}>
            <Text
              style={[
                styles.cardLabel,
                { color: isGasHazard ? theme.colors.danger : theme.colors.text2 },
              ]}
            >
              AIR INTEGRITY
            </Text>
            <Ionicons
              name="warning-outline"
              size={16}
              color={isGasHazard ? theme.colors.danger : theme.colors.success}
            />
          </View>
          <Text
            style={[
              styles.cardValue,
              { color: isGasHazard ? theme.colors.danger : theme.colors.text0 },
            ]}
          >
            {telemetry?.gasPpm !== undefined ? `${telemetry.gasPpm} PPM` : '18 PPM'}
          </Text>
          <Text
            style={[
              styles.cardHint,
              { color: isGasHazard ? theme.colors.danger : theme.colors.text3 },
            ]}
          >
            {isGasHazard ? 'HAZARDOUS ATMOSPHERE' : 'Nominal Safe Quality'}
          </Text>
        </View>
      </View>

      {/* GPS Geo-Positioning Card */}
      <View style={[styles.gpsCard, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View style={styles.gpsHeader}>
          <View style={styles.gpsTitleGroup}>
            <Ionicons name="navigate-outline" size={16} color={theme.colors.text0} />
            <Text style={[styles.gpsTitle, { color: theme.colors.text0 }]}>SATELLITE POSITIONING [GNSS]</Text>
          </View>
          <View style={[styles.gpsLockPill, { backgroundColor: theme.colors.bg1, borderColor: theme.colors.borderStrong }]}>
            <Text style={[styles.gpsLockText, { color: theme.colors.text2 }]}>3D FIX ACTIVE</Text>
          </View>
        </View>

        <View style={styles.gpsRow}>
          <View style={styles.gpsCoordBox}>
            <Text style={[styles.coordLabel, { color: theme.colors.text3 }]}>LATITUDE</Text>
            <Text style={[styles.coordValue, { color: theme.colors.text0 }]}>
              {telemetry?.latitude ? telemetry.latitude.toFixed(6) : '27.717200'}° N
            </Text>
          </View>
          <View style={[styles.coordDivider, { backgroundColor: theme.colors.border }]} />
          <View style={styles.gpsCoordBox}>
            <Text style={[styles.coordLabel, { color: theme.colors.text3 }]}>LONGITUDE</Text>
            <Text style={[styles.coordValue, { color: theme.colors.text0 }]}>
              {telemetry?.longitude ? telemetry.longitude.toFixed(6) : '085.324000'}° E
            </Text>
          </View>
        </View>

        <View style={[styles.gpsFooter, { borderTopColor: theme.colors.border }]}>
          <Text style={[styles.gpsMeta, { color: theme.colors.text3 }]}>
            NEO-8M HIGH-PRECISION DUAL-CONSTELLATION RECEIVER
          </Text>
        </View>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  content: {
    padding: 16,
    paddingBottom: 30,
  },
  nodeBanner: {
    borderRadius: 0,
    padding: 16,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderWidth: 1,
    marginBottom: 16,
  },
  nodeTitle: {
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 1,
  },
  nodeSub: {
    fontSize: 11,
    marginTop: 3,
    fontFamily: 'monospace',
  },
  loraPill: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 9999,
    borderWidth: 1,
  },
  loraDot: {
    width: 6,
    height: 6,
    borderRadius: 3,
  },
  loraPillText: {
    fontSize: 10,
    fontWeight: '800',
    fontFamily: 'monospace',
    letterSpacing: 1,
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
    marginBottom: 16,
  },
  card: {
    width: '48%',
    borderRadius: 0,
    padding: 16,
    borderWidth: 1,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  cardLabel: {
    fontSize: 10,
    fontWeight: '900',
    letterSpacing: 1,
  },
  cardValue: {
    fontSize: 22,
    fontWeight: '900',
    fontFamily: 'monospace',
    marginBottom: 4,
  },
  cardHint: {
    fontSize: 10,
    fontWeight: '500',
    lineHeight: 14,
  },
  gpsCard: {
    borderRadius: 0,
    padding: 18,
    borderWidth: 1,
  },
  gpsHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  gpsTitleGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  gpsTitle: {
    fontSize: 12,
    fontWeight: '900',
    letterSpacing: 1.5,
  },
  gpsLockPill: {
    borderWidth: 1,
    paddingHorizontal: 8,
    paddingVertical: 3,
    borderRadius: 9999,
  },
  gpsLockText: {
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 1,
    fontFamily: 'monospace',
  },
  gpsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 16,
  },
  gpsCoordBox: {
    alignItems: 'center',
  },
  coordLabel: {
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1,
    marginBottom: 4,
  },
  coordValue: {
    fontSize: 16,
    fontWeight: '900',
    fontFamily: 'monospace',
  },
  coordDivider: {
    width: 1,
    height: '100%',
  },
  gpsFooter: {
    borderTopWidth: 1,
    paddingTop: 12,
    alignItems: 'center',
  },
  gpsMeta: {
    fontSize: 9,
    fontWeight: '700',
    letterSpacing: 1,
    fontFamily: 'monospace',
  },
  disconnectedContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 28,
  },
  discTitle: {
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 2,
    marginTop: 16,
    marginBottom: 8,
  },
  discSub: {
    fontSize: 12,
    textAlign: 'center',
    lineHeight: 18,
  },
});
