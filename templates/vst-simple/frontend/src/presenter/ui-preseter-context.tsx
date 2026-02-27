import { defaultParameters } from "@/presenter/parameters";
import {
  useEditorPresenterBase,
  type EditorPresenterBase,
} from "@/hooks/editor-presenter-base";
import { createContext, useContext } from "react";
import type { Parameters } from "./parameters";

type Presenter = EditorPresenterBase<Parameters>;

const uiPresenterContext = createContext<Presenter>({} as Presenter);

// eslint-disable-next-line react-refresh/only-export-components
export function useUiPresenter() {
  return useContext(uiPresenterContext);
}

export const UiPresenterProvider = ({
  children,
}: {
  children: React.ReactNode;
}) => {
  const presenter = useEditorPresenterBase<Parameters>(defaultParameters);
  return (
    <uiPresenterContext.Provider value={presenter}>
      {children}
    </uiPresenterContext.Provider>
  );
};
