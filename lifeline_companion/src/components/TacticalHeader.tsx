import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';

interface TacticalHeaderProps {
  onOpenSettings?: () => void;
}

export const TacticalHeader: React.FC<TacticalHeaderProps> = ({ onOpenSettings }) => {
  const {
    connectionState,
    connectedDevice,
    telemetry,
    disconnectDevice,
    theme,
    themeMode,
    toggleTheme,
  } = useLifeLine();

  const isDark = themeMode === 'dark';

  return (
    <View style={[styles.headerContainer, { backgroundColor: theme.colors.bg0, borderBottomColor: theme.colors.border }]}>
      <View style={styles.contentWrap}>
        {/* Brand Bar */}
        <View style={styles.topRow}>
          <View style={styles.brandGroup}>
            <Ionicons name="radio" size={18} color={theme.colors.text0} style={styles.radioIcon} />
            <Text style={[styles.brandTitle, { color: theme.colors.text0 }]}>LIFELINE</Text>
            <View style={[styles.brandBadge, { borderColor: theme.colors.borderStrong }]}>
              <Text style={[styles.brandBadgeText, { color: theme.colors.text2 }]}>BLE GATT</Text>
            </View>
          </View>

          <View style={styles.rightControls}>
            {/* Dark / Light Mode Quick Toggle */}
            <TouchableOpacity
              style={[
                styles.themeTogglePill,
                { backgroundColor: theme.colors.bg2, borderColor: theme.colors.borderStrong },
              ]}
              onPress={toggleTheme}
              activeOpacity={0.75}
            >
              <Ionicons
                name={isDark ? 'sunny-outline' : 'moon-outline'}
                size={14}
                color={theme.colors.text0}
              />
            </TouchableOpacity>

            {/* Settings Config Gear */}
            {onOpenSettings && (
              <TouchableOpacity
                style={[
                  styles.themeTogglePill,
                  { backgroundColor: theme.colors.bg2, borderColor: theme.colors.borderStrong },
                ]}
                onPress={onOpenSettings}
                activeOpacity={0.75}
              >
                <Ionicons name="settings-outline" size={14} color={theme.colors.text0} />
              </TouchableOpacity>
            )}
          </View>
        </View>

        {/* Link Status & Telemetry Strip */}
        <View
          style={[
            styles.statusRow,
            {
              backgroundColor: theme.colors.bg1,
              borderColor: theme.colors.border,
            },
          ]}
        >
          <View style={styles.statusGroup}>
            <View
              style={[
                styles.statusIndicator,
                connectionState === 'CONNECTED'
                  ? { backgroundColor: theme.colors.success }
                  : connectionState === 'CONNECTING'
                  ? { backgroundColor: theme.colors.solarAmber }
                  : { backgroundColor: theme.colors.text3 },
              ]}
            />
            <Text style={[styles.statusText, { color: theme.colors.text1 }]}>
              {connectionState === 'CONNECTED'
                ? connectedDevice?.name?.toUpperCase() || 'LINK ESTABLISHED'
                : connectionState === 'CONNECTING'
                ? 'ACQUIRING BLE LINK...'
                : connectionState === 'SCANNING'
                ? 'SCANNING 2.4 GHZ BLE...'
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
                    color={telemetry.batteryPct > 20 ? theme.colors.success : theme.colors.danger}
                  />
                  <Text style={[styles.metricValue, { color: theme.colors.text0 }]}>
                    {telemetry.batteryPct}%
                  </Text>
                </View>
              )}

              <TouchableOpacity
                onPress={disconnectDevice}
                style={[
                  styles.disconnectPill,
                  { backgroundColor: theme.colors.bg2, borderColor: theme.colors.borderStrong },
                ]}
                activeOpacity={0.7}
              >
                <Ionicons name="power-outline" size={13} color={theme.colors.text2} />
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
    borderBottomWidth: 1,
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
    paddingTop: 14,
    paddingBottom: 12,
    paddingHorizontal: 16,
  },
  topRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
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
    fontSize: 16,
    fontWeight: '900',
    letterSpacing: 3,
  },
  brandBadge: {
    borderWidth: 1,
    paddingHorizontal: 6,
    paddingVertical: 2,
    borderRadius: 6,
  },
  brandBadgeText: {
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 1.5,
    fontFamily: 'monospace',
  },
  rightControls: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  themeTogglePill: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 9999,
    borderWidth: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    paddingHorizontal: 12,
    borderWidth: 1,
    borderRadius: 10,
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
  statusText: {
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1,
    fontFamily: 'monospace',
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
    fontSize: 11,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  disconnectPill: {
    paddingHorizontal: 8,
    paddingVertical: 3,
    borderRadius: 9999,
    borderWidth: 1,
  },
});
