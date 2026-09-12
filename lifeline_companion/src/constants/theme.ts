// Himalix Labs / LifeLine Design System & Color Palette
// Style Identity: Bugatti-Inspired Austere Luxury Design System
// Reference: docs/DESIGN_PALETTE.md

export type ThemeMode = 'dark' | 'light';

export const DARK_THEME = {
  mode: 'dark' as ThemeMode,
  colors: {
    // Monochromatic Surfaces (Pure Black Canvas Hierarchy)
    bg0: '#000000', // Pure Black Canvas
    bg1: '#0d0d0d', // Soft Surface (inputs, section backdrops)
    bg2: '#141414', // Card Surface
    bg3: '#1f1f1f', // Elevated Surface
    border: '#262626', // Hairlines & Dividers
    borderStrong: '#3a3a3a', // Form & button borders
    borderAccent: 'rgba(255, 255, 255, 0.25)', // Accent Halo

    // Typography & Contrast Hierarchy
    text0: '#ffffff', // Headings, high-impact labels
    text1: '#cccccc', // Primary body text
    text2: '#999999', // Secondary descriptions, metadata
    text3: '#666666', // Muted notes, placeholders

    // Brand & Interactive Accents
    accent: '#ffffff',
    accentDim: '#cccccc',
    buttonFill: '#ffffff',
    buttonText: '#000000',

    // Semantic & Status
    success: '#5fa657',
    successBg: 'rgba(95, 166, 87, 0.12)',
    danger: '#d9534f',
    dangerBg: 'rgba(217, 83, 79, 0.15)',
    warning: '#d4a017',
    warningBg: 'rgba(212, 160, 23, 0.15)',
    info: '#c3d9f3',
    infoBg: 'rgba(195, 217, 243, 0.12)',

    // Ambient Holographic Highlights
    electricIndigo: '#6366f1',
    cyanStream: '#06b6d4',
    digitalViolet: '#8b5cf6',
    solarAmber: '#f59e0b',
  },

  geometry: {
    sharp: 0, // Strict sharp edges for all cards, panels, inputs
    pill: 9999, // Pill silhouette for interactive action buttons & badges
  },

  fonts: {
    mono: 'monospace',
  },
};

export const LIGHT_THEME = {
  mode: 'light' as ThemeMode,
  colors: {
    // Monochromatic Surfaces (Pure White Canvas Hierarchy - docs/DESIGN_PALETTE.md)
    bg0: '#ffffff', // Pure White Canvas
    bg1: '#f7f7f7', // Soft Surface
    bg2: '#eeeeee', // Card Surface
    bg3: '#fcfcfc', // Elevated Surface
    border: '#e0e0e0', // Hairlines & Dividers
    borderStrong: '#888888', // Defined Borders
    borderAccent: 'rgba(0, 0, 0, 0.25)', // Accent Halo

    // Typography & Contrast Hierarchy
    text0: '#000000', // Headings, emphasized brand text
    text1: '#222222', // Primary body text
    text2: '#555555', // Secondary descriptions, metadata
    text3: '#888888', // Muted notes, placeholders

    // Brand & Interactive Accents
    accent: '#000000',
    accentDim: '#333333',
    buttonFill: '#000000',
    buttonText: '#ffffff',

    // Semantic & Status
    success: '#2e7d32',
    successBg: 'rgba(46, 125, 50, 0.1)',
    danger: '#c62828',
    dangerBg: 'rgba(198, 40, 40, 0.1)',
    warning: '#b78103',
    warningBg: 'rgba(183, 129, 3, 0.12)',
    info: '#1565c0',
    infoBg: 'rgba(21, 101, 192, 0.1)',

    // Ambient Holographic Highlights
    electricIndigo: '#4f46e5',
    cyanStream: '#0891b2',
    digitalViolet: '#7c3aed',
    solarAmber: '#d97706',
  },

  geometry: {
    sharp: 0,
    pill: 9999,
  },

  fonts: {
    mono: 'monospace',
  },
};

export const getTheme = (mode: ThemeMode) => (mode === 'light' ? LIGHT_THEME : DARK_THEME);
export const THEME = DARK_THEME;
