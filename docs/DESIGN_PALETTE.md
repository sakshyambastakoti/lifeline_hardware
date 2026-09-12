# Himalix Labs — Design System & Color Palette Reference

> **Style Identity**: *Bugatti-Inspired Austere Luxury Design System*  
> **Aesthetic**: Monochromatic minimalism, ultra-sharp geometry (`0px` radius), pill-shaped interactive elements (`9999px` radius), frosted glassmorphic surfaces, and ambient holographic neon highlights.

---

## 1. Color System & Design Tokens

### Monochromatic Surface Hierarchy (Dark Mode — Default)

| CSS Variable | Hex Code | Color Role & Application |
| :--- | :--- | :--- |
| `--bg-0` | `#000000` | **Pure Black Canvas** — Base canvas and deep background |
| `--bg-1` | `#0d0d0d` | **Soft Surface** — Landing sections, page containers, input backgrounds |
| `--bg-2` | `#141414` | **Card Surface** — Content cards, hover backdrops, interactive surfaces |
| `--bg-3` | `#1f1f1f` | **Elevated Surface** — Dropdown menus, modal sheets, floating panels |
| `--bg-4` / `--border` | `#262626` | **Hairlines & Dividers** — Section dividers, scrollbar track, card frames |
| `--border-strong` | `#3a3a3a` | **Defined Borders** — Form control boundaries, button borders |
| `--border-accent` | `#ffffff40` | **Accent Halo** — Highlighted borders, selected item glow |

### Monochromatic Surface Hierarchy (Light Mode)

Activated via `[data-theme='light']`:

| CSS Variable | Hex Code | Color Role & Application |
| :--- | :--- | :--- |
| `--bg-0` | `#ffffff` | **Pure White Canvas** — Page base background |
| `--bg-1` | `#f7f7f7` | **Soft Surface** — Secondary surface, container fill |
| `--bg-2` | `#eeeeee` | **Card Surface** — Cards and input default fills |
| `--bg-3` | `#fcfcfc` | **Elevated Surface** — Floating elements and modals |
| `--bg-4` / `--border` | `#e0e0e0` | **Hairlines & Dividers** — Light theme dividing lines |
| `--border-strong` | `#888888` | **Defined Borders** — High-contrast borders |
| `--border-accent` | `#00000040` | **Accent Halo** — Active highlight border |

---

## 2. Typography & Contrast Hierarchy

| Token | Dark Mode Hex | Light Mode Hex | Usage |
| :--- | :--- | :--- | :--- |
| `--text-0` | `#ffffff` | `#000000` | Headings, emphasized brand text, high-contrast labels |
| `--text-1` | `#cccccc` | `#222222` | Primary body text, main reading paragraphs |
| `--text-2` | `#999999` | `#555555` | Secondary descriptions, timestamps, metadata |
| `--text-3` | `#666666` | `#888888` | Muted notes, placeholder text, breadcrumb separators |

---

## 3. Brand & Interactive Accents

| Token | Dark Mode Hex | Light Mode Hex | Usage |
| :--- | :--- | :--- | :--- |
| `--accent` | `#ffffff` | `#000000` | Primary action color, button fill (on hover), text selection |
| `--accent-dim` | `#cccccc` | `#333333` | Subdued accent state |
| `--accent-bright` | `#ffffff` | `#000000` | High-impact callout highlight |

---

## 4. Semantic & Status System

| State | Variable | Hex Code | Tint Background / Badges | Usage Example |
| :--- | :--- | :--- | :--- | :--- |
| **Success** | `--success` | `#5fa657` | `rgba(95, 166, 87, 0.1)` / `#4ade8015` | Delivered orders, positive balances, active components |
| **Danger** | `--danger` | `#d9534f` | `rgba(217, 83, 79, 0.1)` / `#f8717115` | Form errors, cancellations, destructive actions |
| **Warning / Gold** | `--warning` | `#d4a017` | `rgba(212, 160, 23, 0.1)` / `#fbbf2415` | Pending status, verification warnings, VIP tier badges |
| **Info / Tech** | `--info` | `#c3d9f3` | `rgba(195, 217, 243, 0.1)` / `#3b82f615` | Desaturated ice-blue for technical links, system advisories |

---

## 5. Ambient & Holographic Accents

Used on the portfolio landing page for the top scroll progress bar, floating ambient orbs, and title text shimmer animations.

| Accent Name | Hex Code | Application |
| :--- | :--- | :--- |
| **Electric Indigo** | `#6366f1` | Hero ambient orb 1, title shimmer sweep |
| **Cyan Stream** | `#06b6d4` | Hero ambient orb 2, scroll indicator midpoint |
| **Digital Violet** | `#8b5cf6` | Hero ambient orb 3, holographic gradient stop |
| **Solar Amber** | `#f59e0b` | Scroll indicator tail, warm highlight |

### Gradient Definitions

```css
/* Fixed Scroll Progress Bar */
background: linear-gradient(90deg, #6366f1, #06b6d4, #8b5cf6, #f59e0b);

/* Hero Title Shimmer Animation */
background: linear-gradient(
  90deg,
  var(--text-0) 0%,
  var(--text-0) 40%,
  #6366f1 50%,
  var(--text-0) 60%,
  var(--text-0) 100%
);

/* Magnetic Button Light Sweep */
background: linear-gradient(
  to right,
  rgba(255, 255, 255, 0) 0%,
  rgba(255, 255, 255, 0.3) 50%,
  rgba(255, 255, 255, 0) 100%
);
```

---

## 6. Glassmorphism & Materials

| Material | Background | Border | Filter |
| :--- | :--- | :--- | :--- |
| **Dark Glass** | `rgba(20, 20, 20, 0.6)` | `1px solid rgba(255, 255, 255, 0.08)` | `backdrop-filter: blur(24px)` |
| **Light Glass** | `rgba(255, 255, 255, 0.7)` | `1px solid rgba(0, 0, 0, 0.1)` | `backdrop-filter: blur(24px)` |
| **Modal Overlay** | `rgba(0, 0, 0, 0.9)` (Dark) / `rgba(255, 255, 255, 0.95)` (Light) | None | — |

---

## 7. Typography Scale & Font Stacks

### Font Families
* **Display Font**: `'Space Grotesk', 'Inter', sans-serif`  
  *(Used for hero headings, section titles, high-impact numbers)*
* **Body Font**: `'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif`  
  *(Used for descriptions, form inputs, labels, continuous reading text)*
* **Monospace Font**: `'JetBrains Mono', monospace`  
  *(Used for button labels, badges, product IDs, specs, telemetry)*

### Size Scale
* `--text-xxs`: `10px`
* `--text-xs`: `11px`
* `--text-sm`: `13px`
* `--text-base`: `16px`
* `--text-lg`: `18px`
* `--text-xl`: `20px`
* `--text-2xl`: `24px`
* `--text-3xl`: `32px`
* `--text-4xl`: `48px`
* `--text-5xl`: `64px`

---

## 8. Geometry & Spacing Rules

* **Strict Sharp Edges**: `--radius: 0px`  
  *All cards, inputs, tables, dropdowns, and modals maintain strict 0px border radius.*
* **Button Pill Silhouette**: `--radius-button: 9999px`  
  *Action buttons (`.btn`, `.btn-primary`, `.btn-outline`) use pill geometry to create contrast against angular layouts.*
* **Functional Shapes**: Radio buttons, loading spinners, and avatars use `50%` radius.

### Spacing Scale
* `--space-1`: `4px`
* `--space-2`: `8px`
* `--space-3`: `12px`
* `--space-4`: `16px`
* `--space-5`: `20px`
* `--space-6`: `24px`
* `--space-8`: `32px`
* `--space-10`: `40px`
* `--space-12`: `48px`
* `--space-16`: `64px`
* `--space-20`: `80px`
* `--space-24`: `96px`
* `--space-section`: `120px`
