import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';

export const SosScreen: React.FC = () => {
  const { connectionState, sosStatus, triggerSos, clearSos, theme } = useLifeLine();

  if (connectionState !== 'CONNECTED') {
    return (
      <View style={[styles.disconnectedContainer, { backgroundColor: theme.colors.bg0 }]}>
        <Ionicons name="warning-outline" size={40} color={theme.colors.text3} />
        <Text style={[styles.discTitle, { color: theme.colors.text1 }]}>DISTRESS BEACON DISARMED</Text>
        <Text style={[styles.discSub, { color: theme.colors.text3 }]}>
          Connect your device to a field node via Radar to arm emergency 433 MHz LoRa distress broadcasting.
        </Text>
      </View>
    );
  }

  return (
    <ScrollView style={[styles.container, { backgroundColor: theme.colors.bg0 }]} contentContainerStyle={styles.content}>
      {/* Active Distress Beacon Console */}
      {sosStatus.isActive ? (
        <View style={[styles.activeSosCard, { backgroundColor: theme.colors.dangerBg, borderColor: theme.colors.danger }]}>
          <View style={styles.beaconHeader}>
            <View style={[styles.pulseDot, { backgroundColor: theme.colors.danger }]} />
            <Text style={[styles.beaconTitle, { color: theme.colors.danger }]}>BROADCASTING DISTRESS PROTOCOL</Text>
          </View>

          <Text style={[styles.alertName, { color: theme.colors.text0 }]}>
            {sosStatus.name} // [CODE {sosStatus.code}]
          </Text>

          {/* Closed-Loop Confirmation Card */}
          <View style={[styles.ackBox, { backgroundColor: theme.colors.bg0, borderColor: theme.colors.borderStrong }]}>
            <View style={styles.ackHeader}>
              <Ionicons
                name={
                  sosStatus.ackStatus === 'DISPATCHED' || sosStatus.ackStatus === 'CONFIRMED'
                    ? 'checkmark-circle-outline'
                    : 'sync-outline'
                }
                size={18}
                color={
                  sosStatus.ackStatus === 'DISPATCHED' || sosStatus.ackStatus === 'CONFIRMED'
                    ? theme.colors.success
                    : theme.colors.solarAmber
                }
              />
              <Text
                style={[
                  styles.ackStatusText,
                  {
                    color:
                      sosStatus.ackStatus === 'DISPATCHED' || sosStatus.ackStatus === 'CONFIRMED'
                        ? theme.colors.success
                        : theme.colors.solarAmber,
                  },
                ]}
              >
                STATUS // {sosStatus.ackStatus}
              </Text>
            </View>

            <Text style={[styles.ackNote, { color: theme.colors.text1 }]}>
              {sosStatus.ackNote || 'Awaiting LoRa ACK acknowledgment from Base Station...'}
            </Text>

            {sosStatus.ackRssi !== null && (
              <View style={[styles.ackMetrics, { borderTopColor: theme.colors.border }]}>
                <Text style={[styles.ackMetricText, { color: theme.colors.text3 }]}>
                  LINK STRENGTH: {sosStatus.ackRssi} DBM • SNR: {sosStatus.ackSnr ?? 0} DB
                </Text>
              </View>
            )}
          </View>

          <TouchableOpacity
            style={[styles.cancelPill, { backgroundColor: theme.colors.bg0, borderColor: theme.colors.danger }]}
            onPress={clearSos}
            activeOpacity={0.8}
          >
            <Ionicons name="close-circle-outline" size={16} color={theme.colors.danger} />
            <Text style={[styles.cancelPillText, { color: theme.colors.danger }]}>
              STAND DOWN // CANCEL DISTRESS
            </Text>
          </TouchableOpacity>
        </View>
      ) : (
        <View style={[styles.standbyBanner, { backgroundColor: theme.colors.bg1, borderColor: theme.colors.border }]}>
          <View style={[styles.standbyDot, { backgroundColor: theme.colors.success }]} />
          <Text style={[styles.standbyText, { color: theme.colors.text1 }]}>
            DISTRESS BEACON ARMED // CLOSED-LOOP LORA PROTOCOL
          </Text>
        </View>
      )}

      {/* Distress Category Selector */}
      <Text style={[styles.sectionHeader, { color: theme.colors.text2 }]}>DEPLOY DISTRESS PROTOCOL</Text>

      {/* 1. General Emergency SOS Button */}
      <TouchableOpacity
        style={[
          styles.sosCard,
          { backgroundColor: theme.colors.bg2, borderColor: theme.colors.borderStrong },
        ]}
        activeOpacity={0.85}
        onPress={() => triggerSos('A', 'GENERAL RESCUE SOS')}
      >
        <View style={styles.cardHeaderRow}>
          <View style={[styles.codePill, { backgroundColor: theme.colors.bg1, borderColor: theme.colors.borderStrong }]}>
            <Text style={[styles.codeText, { color: theme.colors.text2 }]}>CODE // A</Text>
          </View>
          <Ionicons name="alert-circle-outline" size={24} color={theme.colors.danger} />
        </View>
        <Text style={[styles.sosTitle, { color: theme.colors.text0 }]}>GENERAL EMERGENCY SOS</Text>
        <Text style={[styles.sosDesc, { color: theme.colors.text2 }]}>
          Immediate life hazard, structural collapse, flash flood, or trapped party.
        </Text>
      </TouchableOpacity>

      {/* 2. Critical Medical SOS Button */}
      <TouchableOpacity
        style={[
          styles.sosCard,
          { backgroundColor: theme.colors.bg2, borderColor: theme.colors.borderStrong },
        ]}
        activeOpacity={0.85}
        onPress={() => triggerSos('M', 'CRITICAL MEDICAL SOS')}
      >
        <View style={styles.cardHeaderRow}>
          <View style={[styles.codePill, { backgroundColor: theme.colors.bg1, borderColor: theme.colors.borderStrong }]}>
            <Text style={[styles.codeText, { color: theme.colors.text2 }]}>CODE // M</Text>
          </View>
          <Ionicons name="medkit-outline" size={24} color={theme.colors.cyanStream} />
        </View>
        <Text style={[styles.sosTitle, { color: theme.colors.text0 }]}>CRITICAL MEDICAL SOS</Text>
        <Text style={[styles.sosDesc, { color: theme.colors.text2 }]}>
          Severe trauma, high-altitude frostbite, cardiac incident, or stretcher evacuation.
        </Text>
      </TouchableOpacity>

      {/* 3. Wildfire / Toxic Hazard Button */}
      <TouchableOpacity
        style={[
          styles.sosCard,
          { backgroundColor: theme.colors.bg2, borderColor: theme.colors.borderStrong },
        ]}
        activeOpacity={0.85}
        onPress={() => triggerSos('F', 'WILDFIRE / TOXIC HAZARD')}
      >
        <View style={styles.cardHeaderRow}>
          <View style={[styles.codePill, { backgroundColor: theme.colors.bg1, borderColor: theme.colors.borderStrong }]}>
            <Text style={[styles.codeText, { color: theme.colors.text2 }]}>CODE // F</Text>
          </View>
          <Ionicons name="flame-outline" size={24} color={theme.colors.solarAmber} />
        </View>
        <Text style={[styles.sosTitle, { color: theme.colors.text0 }]}>WILDFIRE & TOXIC HAZARD</Text>
        <Text style={[styles.sosDesc, { color: theme.colors.text2 }]}>
          Rapid wildfire perimeter encroachment or hazardous atmospheric gas release.
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  standbyBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    borderWidth: 1,
    paddingVertical: 10,
    paddingHorizontal: 14,
    marginBottom: 20,
    borderRadius: 12,
  },
  standbyDot: {
    width: 6,
    height: 6,
    borderRadius: 3,
  },
  standbyText: {
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1,
    fontFamily: 'monospace',
  },
  sectionHeader: {
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1.5,
    marginBottom: 14,
    paddingHorizontal: 4,
  },
  activeSosCard: {
    borderWidth: 1,
    borderRadius: 18,
    padding: 18,
    marginBottom: 24,
  },
  beaconHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginBottom: 6,
  },
  pulseDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  beaconTitle: {
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1.5,
    fontFamily: 'monospace',
  },
  alertName: {
    fontSize: 16,
    fontWeight: '900',
    letterSpacing: 1,
    marginBottom: 14,
  },
  ackBox: {
    borderRadius: 12,
    padding: 14,
    borderWidth: 1,
    marginBottom: 14,
  },
  ackHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginBottom: 6,
  },
  ackStatusText: {
    fontSize: 11,
    fontWeight: '800',
    fontFamily: 'monospace',
  },
  ackNote: {
    fontSize: 12,
    lineHeight: 18,
    fontFamily: 'monospace',
  },
  ackMetrics: {
    marginTop: 8,
    borderTopWidth: 1,
    paddingTop: 8,
  },
  ackMetricText: {
    fontSize: 10,
    fontFamily: 'monospace',
  },
  cancelPill: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    paddingVertical: 10,
    borderRadius: 9999,
    borderWidth: 1,
  },
  cancelPillText: {
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1,
  },
  sosCard: {
    borderRadius: 16,
    padding: 18,
    marginBottom: 14,
    borderWidth: 1,
  },
  cardHeaderRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  codePill: {
    borderWidth: 1,
    paddingHorizontal: 8,
    paddingVertical: 3,
    borderRadius: 6,
  },
  codeText: {
    fontSize: 10,
    fontWeight: '800',
    fontFamily: 'monospace',
  },
  sosTitle: {
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 1,
    marginBottom: 4,
  },
  sosDesc: {
    fontSize: 11,
    lineHeight: 16,
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
