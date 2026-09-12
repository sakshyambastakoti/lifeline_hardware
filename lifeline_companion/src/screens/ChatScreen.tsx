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
  'LANDSLIDE BLOCKING TRAIL',
  'NEED MEDICAL STRETCHER',
  'SUPPLIES CRITICAL LOW',
  'GROUP SAFE AT SHELTER',
];

export const ChatScreen: React.FC = () => {
  const { connectionState, chatMessages, sendChatMessage, theme } = useLifeLine();
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
        <View
          style={[
            styles.bubble,
            isOut
              ? { backgroundColor: theme.colors.bg3, borderColor: theme.colors.borderStrong }
              : { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border },
          ]}
        >
          <View style={styles.msgHeader}>
            <Text style={[styles.senderText, { color: theme.colors.text0 }]}>{item.sender.toUpperCase()}</Text>
            {item.rssi !== undefined && (
              <Text style={[styles.rssiTag, { color: theme.colors.text3 }]}>{item.rssi} DBM</Text>
            )}
          </View>
          <Text style={[styles.msgText, { color: theme.colors.text1 }]}>{item.text}</Text>
          <View style={styles.msgFooter}>
            <Text style={[styles.timeText, { color: theme.colors.text3 }]}>
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
                  color={item.status === 'CONFIRMED' ? theme.colors.text0 : theme.colors.text3}
                />
                <Text style={[styles.statusText, { color: theme.colors.text2 }]}>
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
      <View style={[styles.disconnectedContainer, { backgroundColor: theme.colors.bg0 }]}>
        <Ionicons name="chatbubbles-outline" size={40} color={theme.colors.text3} />
        <Text style={[styles.discTitle, { color: theme.colors.text1 }]}>LORA MESH TERMINAL OFFLINE</Text>
        <Text style={[styles.discSub, { color: theme.colors.text3 }]}>
          Connect to a LifeLine node via Radar to broadcast tactical text dispatches across mountain terrain.
        </Text>
      </View>
    );
  }

  return (
    <KeyboardAvoidingView
      style={[styles.container, { backgroundColor: theme.colors.bg0 }]}
      behavior={Platform.OS === 'ios' ? 'padding' : undefined}
    >
      {/* Tactical Sub-Header Strip */}
      <View style={[styles.loraHeader, { backgroundColor: theme.colors.bg1, borderBottomColor: theme.colors.border }]}>
        <Ionicons name="radio-outline" size={13} color={theme.colors.text2} />
        <Text style={[styles.loraHeaderText, { color: theme.colors.text2 }]}>
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
            <Text style={[styles.emptyText, { color: theme.colors.text2 }]}>SECURE SESSION INITIALIZED</Text>
            <Text style={[styles.emptySub, { color: theme.colors.text3 }]}>
              Type a field report or choose a tactical macro below to broadcast via LoRa RF.
            </Text>
          </View>
        }
      />

      {/* Tactical Macro Quick Pills */}
      <View style={[styles.macrosContainer, { backgroundColor: theme.colors.bg1, borderTopColor: theme.colors.border }]}>
        <FlatList
          horizontal
          showsHorizontalScrollIndicator={false}
          data={TACTICAL_MACROS}
          renderItem={({ item }) => (
            <TouchableOpacity
              style={[
                styles.macroPill,
                { backgroundColor: theme.colors.bg2, borderColor: theme.colors.borderStrong },
              ]}
              onPress={() => handleMacroPress(item)}
              activeOpacity={0.7}
            >
              <Text style={[styles.macroText, { color: theme.colors.text1 }]}>{item}</Text>
            </TouchableOpacity>
          )}
          keyExtractor={item => item}
        />
      </View>

      {/* Input Console */}
      <View style={[styles.inputBar, { backgroundColor: theme.colors.bg1, borderTopColor: theme.colors.border }]}>
        <TextInput
          style={[
            styles.textInput,
            {
              backgroundColor: theme.colors.bg0,
              borderColor: theme.colors.borderStrong,
              color: theme.colors.text0,
            },
          ]}
          value={inputText}
          onChangeText={setInputText}
          placeholder="TYPE TACTICAL SITREP..."
          placeholderTextColor={theme.colors.text3}
        />
        <TouchableOpacity
          style={[styles.sendPill, { backgroundColor: theme.colors.buttonFill }]}
          onPress={handleSend}
          activeOpacity={0.85}
        >
          <Ionicons name="arrow-up" size={16} color={theme.colors.buttonText} />
        </TouchableOpacity>
      </View>
    </KeyboardAvoidingView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  loraHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    paddingVertical: 8,
    borderBottomWidth: 1,
  },
  loraHeaderText: {
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1.2,
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
    maxWidth: '85%',
    borderRadius: 0,
    padding: 14,
    borderWidth: 1,
  },
  msgHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  senderText: {
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1,
    fontFamily: 'monospace',
  },
  rssiTag: {
    fontSize: 10,
    fontFamily: 'monospace',
  },
  msgText: {
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
    fontSize: 10,
    fontFamily: 'monospace',
  },
  statusIndicator: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  statusText: {
    fontSize: 9,
    fontWeight: '800',
    fontFamily: 'monospace',
    letterSpacing: 0.5,
  },
  macrosContainer: {
    paddingHorizontal: 12,
    paddingVertical: 10,
    borderTopWidth: 1,
  },
  macroPill: {
    paddingHorizontal: 14,
    paddingVertical: 6,
    borderRadius: 9999,
    marginRight: 8,
    borderWidth: 1,
  },
  macroText: {
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 0.8,
    fontFamily: 'monospace',
  },
  inputBar: {
    flexDirection: 'row',
    padding: 12,
    borderTopWidth: 1,
    alignItems: 'center',
    gap: 10,
  },
  textInput: {
    flex: 1,
    borderRadius: 0,
    paddingHorizontal: 14,
    paddingVertical: 10,
    fontSize: 13,
    borderWidth: 1,
    fontFamily: 'monospace',
  },
  sendPill: {
    width: 42,
    height: 42,
    borderRadius: 9999,
    justifyContent: 'center',
    alignItems: 'center',
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
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingTop: 80,
  },
  emptyText: {
    fontSize: 12,
    fontWeight: '900',
    letterSpacing: 2,
  },
  emptySub: {
    fontSize: 11,
    textAlign: 'center',
    marginTop: 6,
    paddingHorizontal: 32,
    lineHeight: 16,
  },
});
