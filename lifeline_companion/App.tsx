import React, { useState } from 'react';
import { View, StyleSheet, TouchableOpacity, Text, SafeAreaView } from 'react-native';
import { StatusBar } from 'expo-status-bar';
import { Ionicons } from '@expo/vector-icons';
import { LifeLineProvider, useLifeLine } from './src/context/LifeLineContext';
import { TacticalHeader } from './src/components/TacticalHeader';
import { RadarScreen } from './src/screens/RadarScreen';
import { DashboardScreen } from './src/screens/DashboardScreen';
import { ChatScreen } from './src/screens/ChatScreen';
import { SosScreen } from './src/screens/SosScreen';
import { SettingsScreen } from './src/screens/SettingsScreen';

type TabKey = 'RADAR' | 'TELEMETRY' | 'CHAT' | 'SOS' | 'SETTINGS';

const MainAppContent: React.FC = () => {
  const [activeTab, setActiveTab] = useState<TabKey>('RADAR');
  const { sosStatus, chatMessages, theme, themeMode } = useLifeLine();

  const unreadChatCount = chatMessages.filter(m => !m.isOutgoing).length;

  return (
    <SafeAreaView style={[styles.safeArea, { backgroundColor: theme.colors.bg0 }]}>
      <StatusBar style={themeMode === 'dark' ? 'light' : 'dark'} />
      <TacticalHeader onOpenSettings={() => setActiveTab('SETTINGS')} />

      <View style={[styles.screenContainer, { backgroundColor: theme.colors.bg0 }]}>
        {activeTab === 'RADAR' && <RadarScreen />}
        {activeTab === 'TELEMETRY' && <DashboardScreen />}
        {activeTab === 'CHAT' && <ChatScreen />}
        {activeTab === 'SOS' && <SosScreen />}
        {activeTab === 'SETTINGS' && <SettingsScreen />}
      </View>

      {/* Austere Luxury Tactical Bottom Navigation */}
      <View style={[styles.navBar, { backgroundColor: theme.colors.bg1, borderTopColor: theme.colors.border }]}>
        {/* Radar Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('RADAR')}
          activeOpacity={0.75}
        >
          <Ionicons
            name={activeTab === 'RADAR' ? 'radio' : 'radio-outline'}
            size={20}
            color={activeTab === 'RADAR' ? theme.colors.text0 : theme.colors.text3}
          />
          <Text
            style={[
              styles.navText,
              { color: activeTab === 'RADAR' ? theme.colors.text0 : theme.colors.text3 },
            ]}
          >
            RADAR
          </Text>
        </TouchableOpacity>

        {/* Telemetry Dashboard Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('TELEMETRY')}
          activeOpacity={0.75}
        >
          <Ionicons
            name={activeTab === 'TELEMETRY' ? 'pulse' : 'pulse-outline'}
            size={20}
            color={activeTab === 'TELEMETRY' ? theme.colors.text0 : theme.colors.text3}
          />
          <Text
            style={[
              styles.navText,
              { color: activeTab === 'TELEMETRY' ? theme.colors.text0 : theme.colors.text3 },
            ]}
          >
            METRICS
          </Text>
        </TouchableOpacity>

        {/* Tactical Chat Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('CHAT')}
          activeOpacity={0.75}
        >
          <View style={styles.iconWithBadge}>
            <Ionicons
              name={activeTab === 'CHAT' ? 'chatbubble-ellipses' : 'chatbubble-ellipses-outline'}
              size={20}
              color={activeTab === 'CHAT' ? theme.colors.text0 : theme.colors.text3}
            />
            {unreadChatCount > 0 && (
              <View style={[styles.badgePill, { backgroundColor: theme.colors.buttonFill }]}>
                <Text style={[styles.badgeText, { color: theme.colors.buttonText }]}>
                  {unreadChatCount}
                </Text>
              </View>
            )}
          </View>
          <Text
            style={[
              styles.navText,
              { color: activeTab === 'CHAT' ? theme.colors.text0 : theme.colors.text3 },
            ]}
          >
            COMMS
          </Text>
        </TouchableOpacity>

        {/* Emergency SOS Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('SOS')}
          activeOpacity={0.75}
        >
          <View style={styles.iconWithBadge}>
            <Ionicons
              name={activeTab === 'SOS' ? 'alert-circle' : 'alert-circle-outline'}
              size={22}
              color={
                sosStatus.isActive
                  ? theme.colors.danger
                  : activeTab === 'SOS'
                  ? theme.colors.text0
                  : theme.colors.text3
              }
            />
            {sosStatus.isActive && (
              <View style={[styles.sosPulse, { backgroundColor: theme.colors.danger }]} />
            )}
          </View>
          <Text
            style={[
              styles.navText,
              {
                color: sosStatus.isActive
                  ? theme.colors.danger
                  : activeTab === 'SOS'
                  ? theme.colors.text0
                  : theme.colors.text3,
              },
            ]}
          >
            DISTRESS
          </Text>
        </TouchableOpacity>

        {/* Settings / Configuration Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('SETTINGS')}
          activeOpacity={0.75}
        >
          <Ionicons
            name={activeTab === 'SETTINGS' ? 'settings' : 'settings-outline'}
            size={20}
            color={activeTab === 'SETTINGS' ? theme.colors.text0 : theme.colors.text3}
          />
          <Text
            style={[
              styles.navText,
              { color: activeTab === 'SETTINGS' ? theme.colors.text0 : theme.colors.text3 },
            ]}
          >
            CONFIG
          </Text>
        </TouchableOpacity>
      </View>
    </SafeAreaView>
  );
};

export default function App() {
  return (
    <LifeLineProvider>
      <MainAppContent />
    </LifeLineProvider>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
  },
  screenContainer: {
    flex: 1,
  },
  navBar: {
    flexDirection: 'row',
    borderTopWidth: 1,
    paddingVertical: 10,
    paddingBottom: 18,
  },
  navItem: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 4,
  },
  navText: {
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 1.5,
    fontFamily: 'monospace',
  },
  iconWithBadge: {
    position: 'relative',
  },
  badgePill: {
    position: 'absolute',
    top: -4,
    right: -8,
    width: 14,
    height: 14,
    borderRadius: 9999,
    justifyContent: 'center',
    alignItems: 'center',
  },
  badgeText: {
    fontSize: 8,
    fontWeight: '900',
    fontFamily: 'monospace',
  },
  sosPulse: {
    position: 'absolute',
    top: -2,
    right: -2,
    width: 7,
    height: 7,
    borderRadius: 3.5,
  },
});
