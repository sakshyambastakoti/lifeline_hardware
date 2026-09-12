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
import { THEME } from './src/constants/theme';

type TabKey = 'RADAR' | 'TELEMETRY' | 'CHAT' | 'SOS';

const MainAppContent: React.FC = () => {
  const [activeTab, setActiveTab] = useState<TabKey>('RADAR');
  const { sosStatus, chatMessages } = useLifeLine();

  const unreadChatCount = chatMessages.filter(m => !m.isOutgoing).length;

  return (
    <SafeAreaView style={styles.safeArea}>
      <StatusBar style="light" />
      <TacticalHeader />

      <View style={styles.screenContainer}>
        {activeTab === 'RADAR' && <RadarScreen />}
        {activeTab === 'TELEMETRY' && <DashboardScreen />}
        {activeTab === 'CHAT' && <ChatScreen />}
        {activeTab === 'SOS' && <SosScreen />}
      </View>

      {/* Austere Luxury Tactical Bottom Navigation */}
      <View style={styles.navBar}>
        {/* Radar Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('RADAR')}
          activeOpacity={0.75}
        >
          <Ionicons
            name={activeTab === 'RADAR' ? 'radio' : 'radio-outline'}
            size={20}
            color={activeTab === 'RADAR' ? THEME.colors.text0 : THEME.colors.text3}
          />
          <Text style={[styles.navText, activeTab === 'RADAR' && styles.navTextActive]}>
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
            color={activeTab === 'TELEMETRY' ? THEME.colors.text0 : THEME.colors.text3}
          />
          <Text style={[styles.navText, activeTab === 'TELEMETRY' && styles.navTextActive]}>
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
              color={activeTab === 'CHAT' ? THEME.colors.text0 : THEME.colors.text3}
            />
            {unreadChatCount > 0 && (
              <View style={styles.badgePill}>
                <Text style={styles.badgeText}>{unreadChatCount}</Text>
              </View>
            )}
          </View>
          <Text style={[styles.navText, activeTab === 'CHAT' && styles.navTextActive]}>
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
              color={sosStatus.isActive ? THEME.colors.danger : activeTab === 'SOS' ? THEME.colors.text0 : THEME.colors.text3}
            />
            {sosStatus.isActive && <View style={styles.sosPulse} />}
          </View>
          <Text
            style={[
              styles.navText,
              activeTab === 'SOS' && styles.navTextActive,
              sosStatus.isActive && styles.navTextSosActive,
            ]}
          >
            DISTRESS
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
    backgroundColor: THEME.colors.bg0,
  },
  screenContainer: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
  },
  navBar: {
    flexDirection: 'row',
    backgroundColor: THEME.colors.bg1,
    borderTopWidth: 1,
    borderTopColor: THEME.colors.border,
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
    color: THEME.colors.text3,
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 1.5,
    fontFamily: THEME.fonts.mono,
  },
  navTextActive: {
    color: THEME.colors.text0,
  },
  navTextSosActive: {
    color: THEME.colors.danger,
  },
  iconWithBadge: {
    position: 'relative',
  },
  badgePill: {
    position: 'absolute',
    top: -4,
    right: -8,
    backgroundColor: THEME.colors.text0,
    width: 14,
    height: 14,
    borderRadius: THEME.geometry.pill,
    justifyContent: 'center',
    alignItems: 'center',
  },
  badgeText: {
    color: THEME.colors.bg0,
    fontSize: 8,
    fontWeight: '900',
    fontFamily: THEME.fonts.mono,
  },
  sosPulse: {
    position: 'absolute',
    top: -2,
    right: -2,
    backgroundColor: THEME.colors.danger,
    width: 7,
    height: 7,
    borderRadius: 3.5,
  },
});
