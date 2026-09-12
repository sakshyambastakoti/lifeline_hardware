import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';
import { THEME } from '../constants/theme';

export const TacticalHeader: React.FC = () => {
  const {
    connectionState,
    connectedDevice,
    telemetry,
    isSimulator,
    setIsSimulator,
    disconnectDevice,
  } = useLifeLine();

  return (
    <View style={styles.headerContainer}>
      {/* Holographic Ambient Accent Top Bar */}
      <View style={styles.holographicBar}>
        <View style={[styles.barSegment, { backgroundColor: THEME.colors.electricIndigo }]} />
        <View style={[styles.barSegment, { backgroundColor: THEME.colors.cyanStream }]} />
        <View style={[styles.barSegment, { backgroundColor: THEME.colors.digitalViolet }]} />
        <View style={[styles.barSegment, { backgroundColor: THEME.colors.solarAmber }]} />
      </View>

      <View style={styles.contentWrap}>
        {/* Brand Bar */}
        <View style={styles.topRow}>
          <View style={styles.brandGroup}>
            <Ionicons name="radio" size={18} color={THEME.colors.text0} style={styles.radioIcon} />
            <Text style={styles.brandTitle}>LIFELINE</Text>
            <View style={styles.brandBadge}>
              <Text style={styles.brandBadgeText}>TACTICAL</Text>
            </View>
          </View>

          <TouchableOpacity
            style={[
              styles.simPill,
              isSimulator ? styles.simPillActive : styles.simPillHardware,
            ]}
            onPress={() => setIsSimulator(!isSimulator)}
            activeOpacity={0.8}
          >
            <View
              style={[
                styles.modeDot,
                { backgroundColor: isSimulator ? THEME.colors.solarAmber : THEME.colors.success },
              ]}
            />
            <Text style={styles.simPillText}>
              {isSimulator ? 'SIMULATOR' : 'HARDWARE BLE'}
            </Text>
          </TouchableOpacity>
        </View>

        {/* Link Status & Telemetry Strip */}
        <View style={styles.statusRow}>
          <View style={styles.statusGroup}>
            <View
              style={[
                styles.statusIndicator,
                connectionState === 'CONNECTED'
                  ? styles.statusConnected
                  : connectionState === 'CONNECTING'
                  ? styles.statusConnecting
                  : styles.statusDisconnected,
              ]}
            />
            <Text style={styles.statusText}>
              {connectionState === 'CONNECTED'
                ? connectedDevice?.name?.toUpperCase() || 'LINK ESTABLISHED'
                : connectionState === 'CONNECTING'
                ? 'ACQUIRING LINK...'
                : connectionState === 'SCANNING'
                ? 'SEARCHING 2.4 GHZ RF...'
                : 'STANDBY'}
            </Text>
          </View>

          {connectionState === 'CONNECTED' && (
            <View style={styles.metricsGroup}>
              {telemetry?.batteryPct !== undefined && (
                <View style={styles.metricItem}>
                  <Ionicons
                    name={
                      telemetry.batteryPct > 40
                        ? 'battery-charging-outline'
                        : 'battery-dead-outline'
                    }
                    size={14}
                    color={telemetry.batteryPct > 20 ? THEME.colors.success : THEME.colors.danger}
                  />
                  <Text style={styles.metricValue}>{telemetry.batteryPct}%</Text>
                </View>
              )}

              <TouchableOpacity
                onPress={disconnectDevice}
                style={styles.disconnectPill}
                activeOpacity={0.7}
              >
                <Ionicons name="power-outline" size={13} color={THEME.colors.text2} />
              </TouchableOpacity>
            </View>
          )}
        </View>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  headerContainer: {
    backgroundColor: THEME.colors.bg0,
    borderBottomWidth: 1,
    borderBottomColor: THEME.colors.border,
  },
  holographicBar: {
    flexDirection: 'row',
    height: 2.5,
    width: '100%',
  },
  barSegment: {
    flex: 1,
    height: '100%',
  },
  contentWrap: {
    paddingTop: 16,
    paddingBottom: 14,
    paddingHorizontal: 18,
  },
  topRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  brandGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  radioIcon: {
    marginRight: 2,
  },
  brandTitle: {
    color: THEME.colors.text0,
    fontSize: 16,
    fontWeight: '900',
    letterSpacing: 3,
  },
  brandBadge: {
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
    paddingHorizontal: 7,
    paddingVertical: 2,
    borderRadius: THEME.geometry.sharp,
  },
  brandBadgeText: {
    color: THEME.colors.text2,
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 1.5,
    fontFamily: THEME.fonts.mono,
  },
  simPill: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    paddingHorizontal: 12,
    paddingVertical: 5,
    borderRadius: THEME.geometry.pill,
    borderWidth: 1,
  },
  simPillActive: {
    backgroundColor: THEME.colors.warningBg,
    borderColor: THEME.colors.warning,
  },
  simPillHardware: {
    backgroundColor: THEME.colors.successBg,
    borderColor: THEME.colors.success,
  },
  modeDot: {
    width: 6,
    height: 6,
    borderRadius: 3,
  },
  simPillText: {
    color: THEME.colors.text0,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1,
    fontFamily: THEME.fonts.mono,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: THEME.colors.bg1,
    paddingVertical: 8,
    paddingHorizontal: 12,
    borderRadius: THEME.geometry.sharp,
    borderWidth: 1,
    borderColor: THEME.colors.border,
  },
  statusGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  statusIndicator: {
    width: 6,
    height: 6,
    borderRadius: 3,
  },
  statusConnected: {
    backgroundColor: THEME.colors.success,
  },
  statusConnecting: {
    backgroundColor: THEME.colors.solarAmber,
  },
  statusDisconnected: {
    backgroundColor: THEME.colors.text3,
  },
  statusText: {
    color: THEME.colors.text1,
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1,
    fontFamily: THEME.fonts.mono,
  },
  metricsGroup: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  metricItem: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  metricValue: {
    color: THEME.colors.text0,
    fontSize: 11,
    fontWeight: '700',
    fontFamily: THEME.fonts.mono,
  },
  disconnectPill: {
    paddingHorizontal: 8,
    paddingVertical: 3,
    backgroundColor: THEME.colors.bg2,
    borderRadius: THEME.geometry.pill,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
  },
});
