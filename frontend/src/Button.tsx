type ButtonProps = {
  onClick?: () => void;
  children: React.ReactNode;
  variant?: "default" | "activated";
  disabled?: boolean;
};

export function Button({
  onClick,
  children,
  variant = "default",
  disabled = false,
}: ButtonProps) {
  const baseStyles =
    "px-4 py-2 rounded-lg font-bold transition-colors duration-200";
  const variantStyles = {
    default: "bg-gray-700 hover:bg-gray-600 text-white disabled:bg-gray-600",
    activated:
      "bg-slate-500 hover:bg-slate-400 text-white disabled:bg-slate-400",
  };

  return (
    <button
      onClick={onClick}
      disabled={disabled}
      className={`${baseStyles} ${variantStyles[variant]}`}>
      {children}
    </button>
  );
}
