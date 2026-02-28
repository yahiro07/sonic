import { defaultParameters } from "@/presenter/parameters";
import {
  useGenericUiPresenter,
  type GenericUiPresenter,
} from "@/hooks/use-generic-ui-presenter";
import { createContext, useContext } from "react";
import type { Parameters } from "./parameters";

type UiPresenter = GenericUiPresenter<Parameters>;

const uiPresenterContext = createContext<UiPresenter>({} as UiPresenter);

// eslint-disable-next-line react-refresh/only-export-components
export function useUiPresenter() {
  return useContext(uiPresenterContext);
}

export const UiPresenterProvider = ({
  children,
}: {
  children: React.ReactNode;
}) => {
  const presenter = useGenericUiPresenter<Parameters>(defaultParameters);
  return (
    <uiPresenterContext.Provider value={presenter}>
      {children}
    </uiPresenterContext.Provider>
  );
};
