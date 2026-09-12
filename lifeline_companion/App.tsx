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

type TabKey = 'RADAR' | 'TELEMETRY' | 'CHAT' | 'SOS';

const MainAppContent: React.FC = () => {
  const [activeTab, setActiveTab] = useState<TabKey>('RADAR');
  const { sosStatus, chatMessages, connectionState } = useLifeLine();

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

      {/* Tactical Bottom Navigation Bar */}
      <View style={styles.navBar}>
        {/* Radar Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('RADAR')}
          activeOpacity={0.7}
        >
          <Ionicons
            name={activeTab === 'RADAR' ? 'radio' : 'radio-outline'}
            size={22}
            color={activeTab === 'RADAR' ? '#06b6d4' : '#64748b'}
          />
          <Text style={[styles.navText, activeTab === 'RADAR' && styles.navTextActive]}>
            RADAR
          </Text>
        </TouchableOpacity>

        {/* Telemetry Dashboard Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('TELEMETRY')}
          activeOpacity={0.7}
        >
          <Ionicons
            name={activeTab === 'TELEMETRY' ? 'pulse' : 'pulse-outline'}
            size={22}
            color={activeTab === 'TELEMETRY' ? '#06b6d4' : '#64748b'}
          />
          <Text style={[styles.navText, activeTab === 'TELEMETRY' && styles.navTextActive]}>
            SENSORS
          </Text>
        </TouchableOpacity>

        {/* Tactical Chat Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('CHAT')}
          activeOpacity={0.7}
        >
          <View style={styles.iconWithBadge}>
            <Ionicons
              name={activeTab === 'CHAT' ? 'chatbubble-ellipses' : 'chatbubble-ellipses-outline'}
              size={22}
              color={activeTab === 'CHAT' ? '#06b6d4' : '#64748b'}
            />
            {unreadChatCount > 0 && (
              <View style={styles.badge}>
                <Text style={styles.badgeText}>{unreadChatCount}</Text>
              </View>
            )}
          </View>
          <Text style={[styles.navText, activeTab === 'CHAT' && styles.navTextActive]}>
            CHAT
          </Text>
        </TouchableOpacity>

        {/* Emergency SOS Tab */}
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => setActiveTab('SOS')}
          activeOpacity={0.7}
        >
          <View style={styles.iconWithBadge}>
            <Ionicons
              name={activeTab === 'SOS' ? 'alert-circle' : 'alert-circle-outline'}
              size={24}
              color={sosStatus.isActive ? '#ef4444' : activeTab === 'SOS' ? '#ef4444' : '#64748b'}
            />
            {sosStatus.isActive && <View style={styles.sosPulse} />}
          </View>
          <Text
            style={[
              styles.navText,
              styles.navTextSos,
              (activeTab === 'SOS' || sosStatus.isActive) && styles.navTextSosActive,
            ]}
          >
            SOS
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
    backgroundColor: '#090d16',
  },
  screenContainer: {
    flex: 1,
  },
  navBar: {
    flexDirection: 'row',
    backgroundColor: '#0c1322',
    borderTopWidth: 1,
    borderTopColor: '#1e293b',
    paddingVertical: 8,
    paddingBottom: 16,
  },
  navItem: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 4,
  },
  navText: {
    color: '#64748b',
    fontSize: 10,
    fontWeight: '700',
    letterSpacing: 0.5,
  },
  navTextActive: {
    color: '#06b6d4',
  },
  navTextSos: {
    color: '#94a3b8',
  },
  navTextSosActive: {
    color: '#ef4444',
  },
  iconWithBadge: {
    position: 'relative',
  },
  badge: {
    position: 'absolute',
    top: -4,
    right: -8,
    backgroundColor: '#06b6d4',
    width: 16,
    height: 16,
    borderRadius: 8,
    justifyContent: 'center',
    alignItems: 'center',
  },
  badgeText: {
    color: '#090d16',
    fontSize: 9,
    fontWeight: '900',
  },
  sosPulse: {
    position: 'absolute',
    top: -2,
    right: -2,
    backgroundColor: '#ef4444',
    width: 8,
    height: 8,
    borderRadius: 4,
  },
});
