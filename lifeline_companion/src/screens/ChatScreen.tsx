import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  FlatList,
  KeyboardAvoidingView,
  Platform,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';
import { ChatMessage } from '../constants/ble';
import { THEME } from '../constants/theme';

const TACTICAL_MACROS = [
  'LANDSLIDE BLOCKING TRAIL',
  'NEED MEDICAL STRETCHER',
  'SUPPLIES CRITICAL LOW',
  'GROUP SAFE AT SHELTER',
];

export const ChatScreen: React.FC = () => {
  const { connectionState, chatMessages, sendChatMessage } = useLifeLine();
  const [inputText, setInputText] = useState('');

  const handleSend = async () => {
    if (!inputText.trim()) return;
    const textToSend = inputText.trim();
    setInputText('');
    await sendChatMessage(textToSend);
  };

  const handleMacroPress = (macro: string) => {
    setInputText(macro);
  };

  const renderMessage = ({ item }: { item: ChatMessage }) => {
    const isOut = item.isOutgoing;

    return (
      <View style={[styles.msgWrapper, isOut ? styles.msgOutWrapper : styles.msgInWrapper]}>
        <View style={[styles.bubble, isOut ? styles.bubbleOut : styles.bubbleIn]}>
          <View style={styles.msgHeader}>
            <Text style={styles.senderText}>{item.sender.toUpperCase()}</Text>
            {item.rssi !== undefined && (
              <Text style={styles.rssiTag}>{item.rssi} DBM</Text>
            )}
          </View>
          <Text style={styles.msgText}>{item.text}</Text>
          <View style={styles.msgFooter}>
            <Text style={styles.timeText}>
              {new Date(item.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
            </Text>
            {isOut && (
              <View style={styles.statusIndicator}>
                <Ionicons
                  name={
                    item.status === 'CONFIRMED'
                      ? 'checkmark-done'
                      : item.status === 'SENT'
                      ? 'checkmark'
                      : item.status === 'PENDING'
                      ? 'time-outline'
                      : 'alert-circle'
                  }
                  size={12}
                  color={item.status === 'CONFIRMED' ? THEME.colors.text0 : THEME.colors.text3}
                />
                <Text style={styles.statusText}>
                  {item.status === 'CONFIRMED' ? 'LORA RELAY ACK' : item.status}
                </Text>
              </View>
            )}
          </View>
        </View>
      </View>
    );
  };

  if (connectionState !== 'CONNECTED') {
    return (
      <View style={styles.disconnectedContainer}>
        <Ionicons name="chatbubbles-outline" size={40} color={THEME.colors.text3} />
        <Text style={styles.discTitle}>LORA MESH TERMINAL OFFLINE</Text>
        <Text style={styles.discSub}>
          Connect to a LifeLine node via Radar to broadcast tactical text dispatches across mountain terrain.
        </Text>
      </View>
    );
  }

  return (
    <KeyboardAvoidingView
      style={styles.container}
      behavior={Platform.OS === 'ios' ? 'padding' : undefined}
    >
      {/* Tactical Sub-Header Strip */}
      <View style={styles.loraHeader}>
        <Ionicons name="radio-outline" size={13} color={THEME.colors.text2} />
        <Text style={styles.loraHeaderText}>
          433 MHZ SX1278 RF RELAY // STORE-AND-FORWARD MESH
        </Text>
      </View>

      {/* Message Stream */}
      <FlatList
        data={chatMessages}
        renderItem={renderMessage}
        keyExtractor={item => item.id}
        contentContainerStyle={styles.listContent}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>SECURE SESSION INITIALIZED</Text>
            <Text style={styles.emptySub}>
              Type a field report or choose a tactical macro below to broadcast via LoRa RF.
            </Text>
          </View>
        }
      />

      {/* Tactical Macro Quick Pills */}
      <View style={styles.macrosContainer}>
        <FlatList
          horizontal
          showsHorizontalScrollIndicator={false}
          data={TACTICAL_MACROS}
          renderItem={({ item }) => (
            <TouchableOpacity
              style={styles.macroPill}
              onPress={() => handleMacroPress(item)}
              activeOpacity={0.7}
            >
              <Text style={styles.macroText}>{item}</Text>
            </TouchableOpacity>
          )}
          keyExtractor={item => item}
        />
      </View>

      {/* Input Console */}
      <View style={styles.inputBar}>
        <TextInput
          style={styles.textInput}
          value={inputText}
          onChangeText={setInputText}
          placeholder="TYPE TACTICAL SITREP..."
          placeholderTextColor={THEME.colors.text3}
        />
        <TouchableOpacity style={styles.sendPill} onPress={handleSend} activeOpacity={0.85}>
          <Ionicons name="arrow-up" size={16} color={THEME.colors.bg0} />
        </TouchableOpacity>
      </View>
    </KeyboardAvoidingView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
  },
  loraHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    backgroundColor: THEME.colors.bg1,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: THEME.colors.border,
  },
  loraHeaderText: {
    color: THEME.colors.text2,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1.2,
    fontFamily: THEME.fonts.mono,
  },
  listContent: {
    padding: 16,
    paddingBottom: 20,
    flexGrow: 1,
  },
  msgWrapper: {
    marginBottom: 12,
    flexDirection: 'row',
  },
  msgInWrapper: {
    justifyContent: 'flex-start',
  },
  msgOutWrapper: {
    justifyContent: 'flex-end',
  },
  bubble: {
    maxWidth: '85%',
    borderRadius: THEME.geometry.sharp,
    padding: 14,
    borderWidth: 1,
  },
  bubbleIn: {
    backgroundColor: THEME.colors.bg2,
    borderColor: THEME.colors.border,
  },
  bubbleOut: {
    backgroundColor: THEME.colors.bg3,
    borderColor: THEME.colors.borderStrong,
  },
  msgHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  senderText: {
    color: THEME.colors.text0,
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1,
    fontFamily: THEME.fonts.mono,
  },
  rssiTag: {
    color: THEME.colors.text3,
    fontSize: 10,
    fontFamily: THEME.fonts.mono,
  },
  msgText: {
    color: THEME.colors.text1,
    fontSize: 13,
    lineHeight: 19,
    letterSpacing: 0.2,
  },
  msgFooter: {
    flexDirection: 'row',
    justifyContent: 'flex-end',
    alignItems: 'center',
    gap: 8,
    marginTop: 8,
  },
  timeText: {
    color: THEME.colors.text3,
    fontSize: 10,
    fontFamily: THEME.fonts.mono,
  },
  statusIndicator: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  statusText: {
    color: THEME.colors.text2,
    fontSize: 9,
    fontWeight: '800',
    fontFamily: THEME.fonts.mono,
    letterSpacing: 0.5,
  },
  macrosContainer: {
    paddingHorizontal: 12,
    paddingVertical: 10,
    backgroundColor: THEME.colors.bg1,
    borderTopWidth: 1,
    borderTopColor: THEME.colors.border,
  },
  macroPill: {
    backgroundColor: THEME.colors.bg2,
    paddingHorizontal: 14,
    paddingVertical: 6,
    borderRadius: THEME.geometry.pill,
    marginRight: 8,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
  },
  macroText: {
    color: THEME.colors.text1,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 0.8,
    fontFamily: THEME.fonts.mono,
  },
  inputBar: {
    flexDirection: 'row',
    padding: 12,
    backgroundColor: THEME.colors.bg1,
    borderTopWidth: 1,
    borderTopColor: THEME.colors.border,
    alignItems: 'center',
    gap: 10,
  },
  textInput: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
    borderRadius: THEME.geometry.sharp,
    paddingHorizontal: 14,
    paddingVertical: 10,
    color: THEME.colors.text0,
    fontSize: 13,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
    fontFamily: THEME.fonts.mono,
  },
  sendPill: {
    width: 42,
    height: 42,
    borderRadius: THEME.geometry.pill,
    backgroundColor: THEME.colors.text0,
    justifyContent: 'center',
    alignItems: 'center',
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
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingTop: 80,
  },
  emptyText: {
    color: THEME.colors.text2,
    fontSize: 12,
    fontWeight: '900',
    letterSpacing: 2,
  },
  emptySub: {
    color: THEME.colors.text3,
    fontSize: 11,
    textAlign: 'center',
    marginTop: 6,
    paddingHorizontal: 32,
    lineHeight: 16,
  },
});
