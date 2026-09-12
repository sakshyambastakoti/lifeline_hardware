import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';

export const SosScreen: React.FC = () => {
  const { connectionState, sosStatus, triggerSos, clearSos } = useLifeLine();

  if (connectionState !== 'CONNECTED') {
    return (
      <View style={styles.disconnectedContainer}>
        <Ionicons name="warning-outline" size={48} color="#ef4444" />
        <Text style={styles.discTitle}>SOS BEACON OFFLINE</Text>
        <Text style={styles.discSub}>
          Connect your phone to a LifeLine transmitter in the Radar tab to enable one-touch LoRa distress broadcasting.
        </Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Active Distress Banner if SOS is triggered */}
      {sosStatus.isActive ? (
        <View style={styles.activeSosCard}>
          <View style={styles.beaconHeader}>
            <View style={styles.pulseDot} />
            <Text style={styles.beaconTitle}>DISTRESS BEACON BROADCASTING</Text>
          </View>

          <Text style={styles.alertName}>{sosStatus.name} (CODE {sosStatus.code})</Text>

          {/* Closed-Loop Confirmation Card */}
          <View style={styles.ackBox}>
            <View style={styles.ackHeader}>
              <Ionicons
                name={
                  sosStatus.ackStatus === 'DISPATCHED' || sosStatus.ackStatus === 'CONFIRMED'
                    ? 'checkmark-circle'
                    : 'sync'
                }
                size={20}
                color={
                  sosStatus.ackStatus === 'DISPATCHED' || sosStatus.ackStatus === 'CONFIRMED'
                    ? '#10b981'
                    : '#f59e0b'
                }
              />
              <Text
                style={[
                  styles.ackStatusText,
                  (sosStatus.ackStatus === 'DISPATCHED' || sosStatus.ackStatus === 'CONFIRMED') &&
                    styles.ackStatusSuccess,
                ]}
              >
                {sosStatus.ackStatus}
              </Text>
            </View>

            <Text style={styles.ackNote}>
              {sosStatus.ackNote || 'Waiting for Base Station LoRa acknowledgment...'}
            </Text>

            {sosStatus.ackRssi !== null && (
              <View style={styles.ackMetrics}>
                <Text style={styles.ackMetricText}>
                  Signal: {sosStatus.ackRssi} dBm • SNR: {sosStatus.ackSnr ?? 0} dB
                </Text>
              </View>
            )}
          </View>

          <TouchableOpacity style={styles.cancelBtn} onPress={clearSos}>
            <Ionicons name="close-circle" size={18} color="#f87171" />
            <Text style={styles.cancelBtnText}>STAND DOWN / CANCEL SOS</Text>
          </TouchableOpacity>
        </View>
      ) : (
        <View style={styles.standbyBanner}>
          <Ionicons name="shield-checkmark" size={20} color="#10b981" />
          <Text style={styles.standbyText}>
            SOS SYSTEM ARMED • Direct LoRa ACK Protection
          </Text>
        </View>
      )}

      {/* Emergency Buttons Grid */}
      <Text style={styles.sectionHeader}>SELECT DISTRESS CATEGORY</Text>

      {/* 1. General Emergency SOS Button */}
      <TouchableOpacity
        style={[styles.sosButton, styles.sosGeneral]}
        activeOpacity={0.8}
        onPress={() => triggerSos('A', 'GENERAL RESCUE SOS')}
      >
        <View style={styles.sosIconBox}>
          <Ionicons name="alert-circle" size={32} color="#ffffff" />
        </View>
        <View style={styles.sosTextBox}>
          <Text style={styles.sosTitle}>GENERAL EMERGENCY SOS</Text>
          <Text style={styles.sosDesc}>
            Immediate danger, trapped group, flash flood, or structural collapse.
          </Text>
        </View>
        <Ionicons name="chevron-forward" size={20} color="#ffffff" />
      </TouchableOpacity>

      {/* 2. Medical Distress Button */}
      <TouchableOpacity
        style={[styles.sosButton, styles.sosMedical]}
        activeOpacity={0.8}
        onPress={() => triggerSos('M', 'CRITICAL MEDICAL SOS')}
      >
        <View style={styles.sosIconBox}>
          <Ionicons name="medkit" size={30} color="#ffffff" />
        </View>
        <View style={styles.sosTextBox}>
          <Text style={styles.sosTitle}>CRITICAL MEDICAL SOS</Text>
          <Text style={styles.sosDesc}>
            Severe trauma, frostbite, cardiac event, or evacuation stretcher required.
          </Text>
        </View>
        <Ionicons name="chevron-forward" size={20} color="#ffffff" />
      </TouchableOpacity>

      {/* 3. Fire / Hazard Distress Button */}
      <TouchableOpacity
        style={[styles.sosButton, styles.sosFire]}
        activeOpacity={0.8}
        onPress={() => triggerSos('F', 'FIRE / TOXIC HAZARD')}
      >
        <View style={styles.sosIconBox}>
          <Ionicons name="flame" size={30} color="#ffffff" />
        </View>
        <View style={styles.sosTextBox}>
          <Text style={styles.sosTitle}>WILDFIRE / HAZARD</Text>
          <Text style={styles.sosDesc}>
            Rapid wildfire spread, hazardous gas leak, or route completely cut off.
          </Text>
        </View>
        <Ionicons name="chevron-forward" size={20} color="#ffffff" />
      </TouchableOpacity>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#090d16',
  },
  content: {
    padding: 16,
    paddingBottom: 30,
  },
  standbyBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    backgroundColor: 'rgba(16, 185, 129, 0.1)',
    borderWidth: 1,
    borderColor: 'rgba(16, 185, 129, 0.3)',
    borderRadius: 8,
    paddingVertical: 10,
    paddingHorizontal: 14,
    marginBottom: 20,
  },
  standbyText: {
    color: '#10b981',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 0.5,
  },
  sectionHeader: {
    color: '#64748b',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1.5,
    marginBottom: 14,
  },
  activeSosCard: {
    backgroundColor: 'rgba(239, 68, 68, 0.15)',
    borderWidth: 2,
    borderColor: '#ef4444',
    borderRadius: 12,
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
    width: 10,
    height: 10,
    borderRadius: 5,
    backgroundColor: '#ef4444',
  },
  beaconTitle: {
    color: '#f87171',
    fontSize: 12,
    fontWeight: '900',
    letterSpacing: 1.5,
  },
  alertName: {
    color: '#ffffff',
    fontSize: 20,
    fontWeight: '900',
    marginBottom: 14,
  },
  ackBox: {
    backgroundColor: '#090d16',
    borderRadius: 8,
    padding: 12,
    borderWidth: 1,
    borderColor: '#1e293b',
    marginBottom: 14,
  },
  ackHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginBottom: 6,
  },
  ackStatusText: {
    color: '#f59e0b',
    fontSize: 13,
    fontWeight: '800',
    fontFamily: 'monospace',
  },
  ackStatusSuccess: {
    color: '#10b981',
  },
  ackNote: {
    color: '#cbd5e1',
    fontSize: 13,
    lineHeight: 18,
  },
  ackMetrics: {
    marginTop: 6,
    borderTopWidth: 1,
    borderTopColor: '#1e293b',
    paddingTop: 6,
  },
  ackMetricText: {
    color: '#64748b',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  cancelBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    backgroundColor: 'rgba(239, 68, 68, 0.2)',
    paddingVertical: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#ef4444',
  },
  cancelBtnText: {
    color: '#f87171',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1,
  },
  sosButton: {
    borderRadius: 12,
    padding: 16,
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 14,
  },
  sosGeneral: {
    backgroundColor: '#dc2626',
  },
  sosMedical: {
    backgroundColor: '#0284c7',
  },
  sosFire: {
    backgroundColor: '#d97706',
  },
  sosIconBox: {
    width: 48,
    height: 48,
    borderRadius: 24,
    backgroundColor: 'rgba(0, 0, 0, 0.2)',
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 14,
  },
  sosTextBox: {
    flex: 1,
  },
  sosTitle: {
    color: '#ffffff',
    fontSize: 15,
    fontWeight: '900',
    letterSpacing: 0.5,
    marginBottom: 2,
  },
  sosDesc: {
    color: 'rgba(255, 255, 255, 0.85)',
    fontSize: 11,
    lineHeight: 15,
  },
  disconnectedContainer: {
    flex: 1,
    backgroundColor: '#090d16',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 24,
  },
  discTitle: {
    color: '#ef4444',
    fontSize: 16,
    fontWeight: '800',
    letterSpacing: 1,
    marginTop: 16,
    marginBottom: 6,
  },
  discSub: {
    color: '#475569',
    fontSize: 13,
    textAlign: 'center',
    lineHeight: 18,
  },
});
