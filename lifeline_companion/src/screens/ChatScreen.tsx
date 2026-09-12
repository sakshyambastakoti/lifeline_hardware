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

const TACTICAL_MACROS = [
  'Landslide blocking trail',
  'Need medical stretcher',
  'Supplies running low',
  'Group safe at shelter',
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
            <Text style={styles.senderText}>{item.sender}</Text>
            {item.rssi !== undefined && (
              <Text style={styles.rssiTag}>{item.rssi} dBm</Text>
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
                  size={14}
                  color={item.status === 'CONFIRMED' ? '#06b6d4' : '#94a3b8'}
                />
                <Text style={styles.statusText}>
                  {item.status === 'CONFIRMED' ? 'LoRa ACK' : item.status}
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
        <Ionicons name="chatbubbles-outline" size={48} color="#475569" />
        <Text style={styles.discTitle}>TACTICAL RADIO OFFLINE</Text>
        <Text style={styles.discSub}>
          Connect to a LifeLine node in the Radar tab to broadcast messages over 433 MHz LoRa.
        </Text>
      </View>
    );
  }

  return (
    <KeyboardAvoidingView
      style={styles.container}
      behavior={Platform.OS === 'ios' ? 'padding' : undefined}
    >
      {/* Tactical LoRa Mesh Banner */}
      <View style={styles.loraHeader}>
        <Ionicons name="radio" size={14} color="#06b6d4" />
        <Text style={styles.loraHeaderText}>
          433 MHz LoRa Mesh • Direct Field-to-Base Relay
        </Text>
      </View>

      {/* Messages List */}
      <FlatList
        data={chatMessages}
        renderItem={renderMessage}
        keyExtractor={item => item.id}
        contentContainerStyle={styles.listContent}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>No transmissions in current session.</Text>
            <Text style={styles.emptySub}>
              Type a field sitrep or select a tactical macro below to broadcast via LoRa.
            </Text>
          </View>
        }
      />

      {/* Tactical Macros for Quick Emergency Dispatch */}
      <View style={styles.macrosContainer}>
        <FlatList
          horizontal
          showsHorizontalScrollIndicator={false}
          data={TACTICAL_MACROS}
          renderItem={({ item }) => (
            <TouchableOpacity
              style={styles.macroChip}
              onPress={() => handleMacroPress(item)}
            >
              <Text style={styles.macroText}>{item}</Text>
            </TouchableOpacity>
          )}
          keyExtractor={item => item}
        />
      </View>

      {/* Input Bar */}
      <View style={styles.inputBar}>
        <TextInput
          style={styles.textInput}
          value={inputText}
          onChangeText={setInputText}
          placeholder="Type field message (broadcasts via LoRa)..."
          placeholderTextColor="#64748b"
        />
        <TouchableOpacity style={styles.sendBtn} onPress={handleSend}>
          <Ionicons name="paper-plane" size={18} color="#090d16" />
        </TouchableOpacity>
      </View>
    </KeyboardAvoidingView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#090d16',
  },
  loraHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    backgroundColor: '#0f172a',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#1e293b',
  },
  loraHeaderText: {
    color: '#06b6d4',
    fontSize: 11,
    fontWeight: '700',
    fontFamily: 'monospace',
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
    maxWidth: '82%',
    borderRadius: 12,
    padding: 12,
    borderWidth: 1,
  },
  bubbleIn: {
    backgroundColor: '#0f172a',
    borderColor: '#1e293b',
  },
  bubbleOut: {
    backgroundColor: '#1e293b',
    borderColor: '#334155',
  },
  msgHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 4,
  },
  senderText: {
    color: '#06b6d4',
    fontSize: 11,
    fontWeight: '800',
  },
  rssiTag: {
    color: '#64748b',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  msgText: {
    color: '#f8fafc',
    fontSize: 14,
    lineHeight: 20,
  },
  msgFooter: {
    flexDirection: 'row',
    justifyContent: 'flex-end',
    alignItems: 'center',
    gap: 6,
    marginTop: 6,
  },
  timeText: {
    color: '#64748b',
    fontSize: 10,
  },
  statusIndicator: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 3,
  },
  statusText: {
    color: '#94a3b8',
    fontSize: 10,
    fontWeight: '600',
  },
  macrosContainer: {
    paddingHorizontal: 12,
    paddingVertical: 8,
    backgroundColor: '#0c1322',
    borderTopWidth: 1,
    borderTopColor: '#1e293b',
  },
  macroChip: {
    backgroundColor: '#1e293b',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 16,
    marginRight: 8,
    borderWidth: 1,
    borderColor: '#334155',
  },
  macroText: {
    color: '#cbd5e1',
    fontSize: 12,
    fontWeight: '600',
  },
  inputBar: {
    flexDirection: 'row',
    padding: 12,
    backgroundColor: '#0f172a',
    borderTopWidth: 1,
    borderTopColor: '#1e293b',
    alignItems: 'center',
    gap: 10,
  },
  textInput: {
    flex: 1,
    backgroundColor: '#090d16',
    borderRadius: 8,
    paddingHorizontal: 14,
    paddingVertical: 10,
    color: '#f8fafc',
    fontSize: 14,
    borderWidth: 1,
    borderColor: '#1e293b',
  },
  sendBtn: {
    width: 44,
    height: 44,
    borderRadius: 8,
    backgroundColor: '#06b6d4',
    justifyContent: 'center',
    alignItems: 'center',
  },
  disconnectedContainer: {
    flex: 1,
    backgroundColor: '#090d16',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 24,
  },
  discTitle: {
    color: '#94a3b8',
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
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingTop: 80,
  },
  emptyText: {
    color: '#94a3b8',
    fontSize: 14,
    fontWeight: '700',
  },
  emptySub: {
    color: '#475569',
    fontSize: 12,
    textAlign: 'center',
    marginTop: 4,
    paddingHorizontal: 30,
  },
});
