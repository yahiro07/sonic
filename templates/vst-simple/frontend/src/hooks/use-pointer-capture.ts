export type PointerPoint = { x: number; y: number };

type PointerCaptureListener = {
  onDown?(point: PointerPoint): void;
  onMove?(point: PointerPoint): void;
  onUp?(point: PointerPoint): void;
  onCancel?(point: PointerPoint): void;
};

export function usePointerCapture(listener: PointerCaptureListener) {
  return (e0: React.PointerEvent<HTMLElement>) => {
    const el = e0.currentTarget as HTMLElement;
    const rect = el.getBoundingClientRect();
    let isSubscriptionActive = false;

    const getPointerPoint = (e: PointerEvent): PointerPoint => {
      return { x: e.clientX - rect.left, y: e.clientY - rect.top };
    };

    const onPointerMove = (e: PointerEvent) => {
      if (e.pointerId === e0.pointerId) {
        listener.onMove?.(getPointerPoint(e));
      }
    };
    const onPointerUp = (e: PointerEvent) => {
      if (e.pointerId === e0.pointerId) {
        listener.onUp?.(getPointerPoint(e));
        unsubscribeListeners();
      }
    };

    const onPointerCancel = (e: PointerEvent) => {
      if (e.pointerId === e0.pointerId) {
        listener.onCancel?.(getPointerPoint(e));
        unsubscribeListeners();
      }
    };

    const subscribeListeners = () => {
      window.addEventListener("pointermove", onPointerMove);
      window.addEventListener("pointerup", onPointerUp);
      window.addEventListener("pointercancel", onPointerCancel);
      try {
        el.setPointerCapture(e0.pointerId);
      } catch {
        //ignore
      }
      isSubscriptionActive = true;
    };

    const unsubscribeListeners = () => {
      if (isSubscriptionActive) {
        try {
          el.releasePointerCapture(e0.pointerId);
        } catch {
          //ignore
        }
        window.removeEventListener("pointermove", onPointerMove);
        window.removeEventListener("pointerup", onPointerUp);
        window.removeEventListener("pointercancel", onPointerCancel);
        isSubscriptionActive = false;
      }
    };

    listener.onDown?.(getPointerPoint(e0.nativeEvent));
    subscribeListeners();
  };
}
