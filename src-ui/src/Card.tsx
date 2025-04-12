type CardProps = {
  title: string;
  children: React.ReactNode;
};

export function Card({ title, children }: CardProps) {
  return (
    <div className="bg-gray-800 text-white p-5 rounded-xl shadow-xl">
      <h2 className="text-xl font-semibold text-blue-300 mb-3">{title}</h2>
      {children}
    </div>
  );
}
