/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
  darkMode: "class",
  theme: {
    extend: {
      colors: {
        surface: {
          900: "#0f0f0f",
          800: "#1a1a1a",
          700: "#262626",
          600: "#333333",
        },
        accent: {
          green: "#22c55e",
          red: "#ef4444",
          yellow: "#eab308",
          cyan: "#06b6d4",
        },
        vu: {
          green: "#22c55e",
          yellow: "#eab308",
          red: "#ef4444",
        },
      },
      fontFamily: {
        mono: ["JetBrains Mono", "Fira Code", "monospace"],
      },
    },
  },
  plugins: [],
};
