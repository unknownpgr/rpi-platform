import { useEffect, useState } from "react";
import { Server } from "./core/server";

export function useServer(server: Server) {
  const [_, render] = useState(0);
  useEffect(() => {
    return server.addListener(() => {
      render((prev) => prev + 1);
    });
  }, [server]);
}
