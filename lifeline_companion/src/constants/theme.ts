// Himalix Labs / LifeLine Design System & Color Palette
// Style Identity: Bugatti-Inspired Austere Luxury Design System
// Reference: docs/DESIGN_PALETTE.md

export const THEME = {
  colors: {
    // Monochromatic Surfaces (Pure Black Canvas Hierarchy)
    bg0: '#000000', // Pure Black Canvas
    bg1: '#0d0d0d', // Soft Surface (inputs, section backdrops)
    bg2: '#141414', // Card Surface
    bg3: '#1f1f1f', // Elevated Surface (floating panels, popovers)
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
    accentHover: 'rgba(255, 255, 255, 0.1)',

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
