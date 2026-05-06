import { useState } from "react";

interface MetadataConfigProps {
  onClose: () => void;
  onSetSource: (source: MetadataSourceConfig) => void;
}

interface MetadataSourceConfig {
  type: "None" | "WindowTitle" | "Application" | "Network" | "TextFile" | "HttpUrl";
  value?: string;
}

export default function MetadataConfig({ onClose, onSetSource }: MetadataConfigProps) {
  const [sourceType, setSourceType] = useState<string>("None");
  const [value, setValue] = useState("");

  const handleApply = () => {
    if (sourceType === "None") {
      onSetSource({ type: "None" });
    } else if (sourceType === "WindowTitle") {
      onSetSource({ type: "WindowTitle", value: value || "Active Window" });
    } else if (sourceType === "Application") {
      onSetSource({ type: "Application", value: value || "" });
    } else if (sourceType === "Network") {
      onSetSource({ type: "Network" });
    } else if (sourceType === "TextFile") {
      onSetSource({ type: "TextFile", value: value || "" });
    } else if (sourceType === "HttpUrl") {
      onSetSource({ type: "HttpUrl", value: value || "" });
    }
    onClose();
  };

  return (
    <div className="fixed inset-0 bg-black/60 flex items-center justify-center z-50">
      <div className="bg-zinc-900 border border-zinc-700 rounded-lg w-[360px] shadow-2xl">
        <div className="px-4 py-3 border-b border-zinc-800 flex items-center justify-between">
          <h3 className="text-xs font-semibold text-zinc-300 uppercase tracking-wider">
            Metadata Source
          </h3>
          <button
            onClick={onClose}
            className="text-zinc-500 hover:text-zinc-300 text-sm leading-none"
          >
            ✕
          </button>
        </div>

        <div className="p-4 space-y-3">
          <div>
            <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
              Source Type
            </label>
            <select
              value={sourceType}
              onChange={(e) => setSourceType(e.target.value)}
              className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
            >
              <option value="None">None (manual only)</option>
              <option value="WindowTitle">Window Title</option>
              <option value="Application">Application Name</option>
              <option value="Network">Network Listener</option>
              <option value="TextFile">Text File</option>
              <option value="HttpUrl">HTTP URL</option>
            </select>
          </div>

          {(sourceType === "WindowTitle" ||
            sourceType === "Application" ||
            sourceType === "TextFile" ||
            sourceType === "HttpUrl") && (
            <div>
              <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                {sourceType === "TextFile"
                  ? "File Path"
                  : sourceType === "HttpUrl"
                    ? "URL"
                    : "Value"}
              </label>
              <input
                type="text"
                value={value}
                onChange={(e) => setValue(e.target.value)}
                className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
                placeholder={
                  sourceType === "TextFile"
                    ? "/path/to/metadata.txt"
                    : sourceType === "HttpUrl"
                      ? "https://..."
                      : "Enter value"
                }
              />
            </div>
          )}

          <div className="flex gap-2 pt-1">
            <button
              onClick={onClose}
              className="flex-1 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-zinc-400 rounded text-xs font-medium transition-colors border border-zinc-700"
            >
              Cancel
            </button>
            <button
              onClick={handleApply}
              className="flex-1 px-3 py-1.5 bg-emerald-700 hover:bg-emerald-600 text-white rounded text-xs font-semibold transition-colors"
            >
              Apply
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
