type RootPointerEvent = PointerEvent;

export type PointerPoint = { x: number; y: number };

export function startPointerCaptureSession(
  e0: RootPointerEvent,
  args: {
    onDown?(point: PointerPoint): void;
    onMove?(point: PointerPoint): void;
    onUp?(point: PointerPoint): void;
    onCancel?(point: PointerPoint): void;
  },
) {
  const { onDown, onMove, onUp, onCancel } = args;
  const el = e0.currentTarget as HTMLElement;
  const rect = el.getBoundingClientRect();
  let isSubscriptionActive = false;

  const getPointerPoint = (e: PointerEvent): PointerPoint => {
    return { x: e.clientX - rect.left, y: e.clientY - rect.top };
  };

  const onPointerMove = (e: PointerEvent) => {
    if (e.pointerId === e0.pointerId) {
      onMove?.(getPointerPoint(e));
    }
  };
  const onPointerUp = (e: PointerEvent) => {
    if (e.pointerId === e0.pointerId) {
      onUp?.(getPointerPoint(e));
      unsubscribeListeners();
    }
  };

  const onPointerCancel = (e: PointerEvent) => {
    if (e.pointerId === e0.pointerId) {
      onCancel?.(getPointerPoint(e));
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

  onDown?.(getPointerPoint(e0));
  subscribeListeners();
}
