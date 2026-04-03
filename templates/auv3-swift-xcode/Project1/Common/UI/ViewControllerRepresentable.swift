import SwiftUI

#if os(iOS) || os(visionOS)

  struct AUViewControllerUI: UIViewControllerRepresentable {
    var auViewController: UIViewController?

    init(viewController: UIViewController?) {
      self.auViewController = viewController
    }

    func makeUIViewController(context: Context) -> UIViewController {
      guard let auViewController = self.auViewController else {
        return UIViewController()
      }

      let viewController = UIViewController()
      viewController.addChild(auViewController)

      let frame: CGRect = viewController.view.bounds
      auViewController.view.frame = frame

      viewController.view.addSubview(auViewController.view)
      auViewController.didMove(toParent: viewController)
      return viewController
    }

    func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
      // No op
    }
  }
#elseif os(macOS)

  final class ResizingContainerView: NSView {
    weak var hostedView: NSView?

    private func sync() {
      hostedView?.frame = bounds
    }

    override func layout() {
      super.layout()
      sync()
    }

    override func setFrameSize(_ newSize: NSSize) {
      super.setFrameSize(newSize)
      sync()
    }
  }

  final class ContainerViewController: NSViewController {
    private var hosted: NSViewController?

    override func loadView() {
      self.view = ResizingContainerView()
    }

    func setHosted(_ vc: NSViewController?) {
      if hosted === vc { return }

      if let hosted {
        hosted.view.removeFromSuperview()
        hosted.removeFromParent()
        (view as? ResizingContainerView)?.hostedView = nil
      }
      hosted = vc
      guard let vc else { return }

      addChild(vc)

      vc.view.translatesAutoresizingMaskIntoConstraints = true
      vc.view.frame = view.bounds

      view.addSubview(vc.view)
      (view as? ResizingContainerView)?.hostedView = vc.view

      vc.view.layoutSubtreeIfNeeded()
    }
  }

  struct AUViewControllerUI: NSViewControllerRepresentable {
    var auViewController: NSViewController?

    init(viewController: NSViewController?) {
      self.auViewController = viewController
    }

    func makeNSViewController(context: Context) -> ContainerViewController {
      let container = ContainerViewController()
      container.setHosted(auViewController)
      return container
    }

    func updateNSViewController(_ nsViewController: ContainerViewController, context: Context) {
      nsViewController.setHosted(auViewController)
    }
  }

#endif
