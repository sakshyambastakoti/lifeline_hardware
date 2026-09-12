import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';
import { THEME } from '../constants/theme';

export const SosScreen: React.FC = () => {
  const { connectionState, sosStatus, triggerSos, clearSos } = useLifeLine();

  if (connectionState !== 'CONNECTED') {
    return (
      <View style={styles.disconnectedContainer}>
        <Ionicons name="warning-outline" size={40} color={THEME.colors.text3} />
        <Text style={styles.discTitle}>DISTRESS BEACON DISARMED</Text>
        <Text style={styles.discSub}>
          Connect your device to a field node via Radar to arm emergency 433 MHz LoRa distress broadcasting.
        </Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Active Distress Beacon Console */}
      {sosStatus.isActive ? (
        <View style={styles.activeSosCard}>
          <View style={styles.beaconHeader}>
            <View style={styles.pulseDot} />
            <Text style={styles.beaconTitle}>BROADCASTING DISTRESS PROTOCOL</Text>
          </View>

          <Text style={styles.alertName}>{sosStatus.name} // [CODE {sosStatus.code}]</Text>

          {/* Closed-Loop Confirmation Card */}
          <View style={styles.ackBox}>
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
                    ? THEME.colors.success
                    : THEME.colors.solarAmber
                }
              />
              <Text
                style={[
                  styles.ackStatusText,
                  (sosStatus.ackStatus === 'DISPATCHED' || sosStatus.ackStatus === 'CONFIRMED') &&
                    styles.ackStatusSuccess,
                ]}
              >
                STATUS // {sosStatus.ackStatus}
              </Text>
            </View>

            <Text style={styles.ackNote}>
              {sosStatus.ackNote || 'Awaiting LoRa ACK acknowledgment from Base Station...'}
            </Text>

            {sosStatus.ackRssi !== null && (
              <View style={styles.ackMetrics}>
                <Text style={styles.ackMetricText}>
                  LINK STRENGTH: {sosStatus.ackRssi} DBM • SNR: {sosStatus.ackSnr ?? 0} DB
                </Text>
              </View>
            )}
          </View>

          <TouchableOpacity style={styles.cancelPill} onPress={clearSos} activeOpacity={0.8}>
            <Ionicons name="close-circle-outline" size={16} color={THEME.colors.danger} />
            <Text style={styles.cancelPillText}>STAND DOWN // CANCEL DISTRESS</Text>
          </TouchableOpacity>
        </View>
      ) : (
        <View style={styles.standbyBanner}>
          <View style={styles.standbyDot} />
          <Text style={styles.standbyText}>
            DISTRESS BEACON ARMED // CLOSED-LOOP LORA PROTOCOL
          </Text>
        </View>
      )}

      {/* Distress Category Selector */}
      <Text style={styles.sectionHeader}>DEPLOY DISTRESS PROTOCOL</Text>

      {/* 1. General Emergency SOS Button */}
      <TouchableOpacity
        style={[styles.sosCard, styles.sosGeneralCard]}
        activeOpacity={0.85}
        onPress={() => triggerSos('A', 'GENERAL RESCUE SOS')}
      >
        <View style={styles.cardHeaderRow}>
          <View style={styles.codePill}>
            <Text style={styles.codeText}>CODE // A</Text>
          </View>
          <Ionicons name="alert-circle-outline" size={24} color={THEME.colors.text0} />
        </View>
        <Text style={styles.sosTitle}>GENERAL EMERGENCY SOS</Text>
        <Text style={styles.sosDesc}>
          Immediate life hazard, structural collapse, flash flood, or trapped party.
        </Text>
      </TouchableOpacity>

      {/* 2. Critical Medical SOS Button */}
      <TouchableOpacity
        style={[styles.sosCard, styles.sosMedicalCard]}
        activeOpacity={0.85}
        onPress={() => triggerSos('M', 'CRITICAL MEDICAL SOS')}
      >
        <View style={styles.cardHeaderRow}>
          <View style={styles.codePill}>
            <Text style={styles.codeText}>CODE // M</Text>
          </View>
          <Ionicons name="medkit-outline" size={24} color={THEME.colors.text0} />
        </View>
        <Text style={styles.sosTitle}>CRITICAL MEDICAL SOS</Text>
        <Text style={styles.sosDesc}>
          Severe trauma, high-altitude frostbite, cardiac incident, or stretcher evacuation.
        </Text>
      </TouchableOpacity>

      {/* 3. Wildfire / Toxic Hazard Button */}
      <TouchableOpacity
        style={[styles.sosCard, styles.sosFireCard]}
        activeOpacity={0.85}
        onPress={() => triggerSos('F', 'WILDFIRE / TOXIC HAZARD')}
      >
        <View style={styles.cardHeaderRow}>
          <View style={styles.codePill}>
            <Text style={styles.codeText}>CODE // F</Text>
          </View>
          <Ionicons name="flame-outline" size={24} color={THEME.colors.text0} />
        </View>
        <Text style={styles.sosTitle}>WILDFIRE & TOXIC HAZARD</Text>
        <Text style={styles.sosDesc}>
          Rapid wildfire perimeter encroachment or hazardous atmospheric gas release.
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  standbyBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    backgroundColor: THEME.colors.bg1,
    borderWidth: 1,
    borderColor: THEME.colors.border,
    paddingVertical: 10,
    paddingHorizontal: 14,
    marginBottom: 20,
    borderRadius: THEME.geometry.sharp,
  },
  standbyDot: {
    width: 6,
    height: 6,
    borderRadius: 3,
    backgroundColor: THEME.colors.success,
  },
  standbyText: {
    color: THEME.colors.text1,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1,
    fontFamily: THEME.fonts.mono,
  },
  sectionHeader: {
    color: THEME.colors.text2,
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1.5,
    marginBottom: 14,
    paddingHorizontal: 4,
  },
  activeSosCard: {
    backgroundColor: THEME.colors.dangerBg,
    borderWidth: 1,
    borderColor: THEME.colors.danger,
    borderRadius: THEME.geometry.sharp,
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
    backgroundColor: THEME.colors.danger,
  },
  beaconTitle: {
    color: THEME.colors.danger,
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1.5,
    fontFamily: THEME.fonts.mono,
  },
  alertName: {
    color: THEME.colors.text0,
    fontSize: 16,
    fontWeight: '900',
    letterSpacing: 1,
    marginBottom: 14,
  },
  ackBox: {
    backgroundColor: THEME.colors.bg0,
    borderRadius: THEME.geometry.sharp,
    padding: 14,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
    marginBottom: 14,
  },
  ackHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginBottom: 6,
  },
  ackStatusText: {
    color: THEME.colors.solarAmber,
    fontSize: 11,
    fontWeight: '800',
    fontFamily: THEME.fonts.mono,
  },
  ackStatusSuccess: {
    color: THEME.colors.success,
  },
  ackNote: {
    color: THEME.colors.text1,
    fontSize: 12,
    lineHeight: 18,
    fontFamily: THEME.fonts.mono,
  },
  ackMetrics: {
    marginTop: 8,
    borderTopWidth: 1,
    borderTopColor: THEME.colors.border,
    paddingTop: 8,
  },
  ackMetricText: {
    color: THEME.colors.text3,
    fontSize: 10,
    fontFamily: THEME.fonts.mono,
  },
  cancelPill: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    backgroundColor: THEME.colors.bg0,
    paddingVertical: 10,
    borderRadius: THEME.geometry.pill,
    borderWidth: 1,
    borderColor: THEME.colors.danger,
  },
  cancelPillText: {
    color: THEME.colors.danger,
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1,
  },
  sosCard: {
    borderRadius: THEME.geometry.sharp,
    padding: 18,
    marginBottom: 14,
    borderWidth: 1,
  },
  sosGeneralCard: {
    backgroundColor: THEME.colors.bg2,
    borderColor: THEME.colors.borderStrong,
  },
  sosMedicalCard: {
    backgroundColor: THEME.colors.bg2,
    borderColor: THEME.colors.borderStrong,
  },
  sosFireCard: {
    backgroundColor: THEME.colors.bg2,
    borderColor: THEME.colors.borderStrong,
  },
  cardHeaderRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  codePill: {
    backgroundColor: THEME.colors.bg1,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
    paddingHorizontal: 8,
    paddingVertical: 3,
    borderRadius: THEME.geometry.sharp,
  },
  codeText: {
    color: THEME.colors.text2,
    fontSize: 10,
    fontWeight: '800',
    fontFamily: THEME.fonts.mono,
  },
  sosTitle: {
    color: THEME.colors.text0,
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 1,
    marginBottom: 4,
  },
  sosDesc: {
    color: THEME.colors.text2,
    fontSize: 11,
    lineHeight: 16,
  },
  disconnectedContainer: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 28,
  },
  discTitle: {
    color: THEME.colors.text1,
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 2,
    marginTop: 16,
    marginBottom: 8,
  },
  discSub: {
    color: THEME.colors.text3,
    fontSize: 12,
    textAlign: 'center',
    lineHeight: 18,
  },
});
