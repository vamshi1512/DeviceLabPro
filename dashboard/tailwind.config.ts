import type { Config } from "tailwindcss";

export default {
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      fontFamily: {
        display: ["Space Grotesk", "sans-serif"],
        mono: ["IBM Plex Mono", "monospace"],
      },
      colors: {
        shell: {
          950: "#060d18",
          900: "#0a1628",
          800: "#12233f",
          700: "#1a3560",
        },
        accent: {
          400: "#68d1ff",
          500: "#36b7ff",
          600: "#1090dd",
        },
        signal: {
          ok: "#45d483",
          warn: "#f7b955",
          error: "#fb6c67",
        },
      },
      boxShadow: {
        panel: "0 28px 60px rgba(0, 0, 0, 0.35)",
      },
      backgroundImage: {
        "dashboard-grid":
          "linear-gradient(rgba(137,195,255,0.06) 1px, transparent 1px), linear-gradient(90deg, rgba(137,195,255,0.06) 1px, transparent 1px)",
      },
      keyframes: {
        drift: {
          "0%, 100%": { transform: "translate3d(0,0,0)" },
          "50%": { transform: "translate3d(0,-8px,0)" },
        },
        pulseGlow: {
          "0%, 100%": { opacity: "0.5" },
          "50%": { opacity: "0.95" },
        },
      },
      animation: {
        drift: "drift 8s ease-in-out infinite",
        pulseGlow: "pulseGlow 3s ease-in-out infinite",
      },
    },
  },
  plugins: [],
} satisfies Config;
