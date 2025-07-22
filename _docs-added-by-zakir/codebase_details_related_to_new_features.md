# Codebase Details for New Features

This document outlines the key areas of the LibreOffice codebase that will be relevant for implementing the new features described in the PRD. 

## 1. Cloud Storage and Authentication ("Save to Cloud", "Refresh from Cloud", OAuth)

### Relevant Modules:
- **`sfx2` (Legacy Framework):** This module is critical because it handles the core document loading and saving logic.
    - The file `sfx2/source/doc/docfile.cxx` contains the `SfxMedium` class, which manages all aspects of file I/O. We will likely need to extend this class or create a new mechanism that hooks into it to handle the "Save to Cloud" and "Refresh from Cloud" functionality.
    - The existing "Save to Remote" feature, while more complex than what we need, can serve as a reference. Understanding its implementation will help us avoid pitfalls.
    - **Deeper Dive:** `SfxMedium` acts as a wrapper around the actual storage medium, whether it's a local file, a stream, or a remote resource accessed via UCB (Universal Content Broker).
        - The `Commit()` method is the key function for saving changes. It handles the transfer of data from a temporary location to the final destination. We will need to create a new implementation of this process for our cloud storage.
        - The `GetStorage()` method is used to retrieve an `embed::XStorage` object, which represents the document's contents. We can create our own storage implementation that reads from and writes to S3.
        - The class makes extensive use of the UCB, which provides a unified API for accessing different types of content. We can leverage this by creating a UCB provider for our cloud service, though a simpler approach might be to handle the S3 communication directly within a custom `SfxMedium`-like class.
- **`vcl` (Visual Class Library):** This module provides the UI components.
    - We will need to use `vcl` to create the new "Save to Cloud" and "Refresh from Cloud" menu items in the "File" menu.
    - For the OAuth-based login, we will need to create a new dialog using `vcl` widgets. This dialog will likely need to embed a web view to handle the OAuth flow, which might require investigating how to integrate a browser component into a `vcl` window.
    - **Deeper Dive:**
        - `vcl/source/window/menu.cxx` defines the `Menu` class, which is the base class for all menus. It manages a list of `MenuItemData` objects, which represent the individual items in the menu.
        - To add our "Save to Cloud" menu item, we will need to use the `InsertItem` method. We can also set an accelerator key using `SetAccelKey` and associate a command string using `SetItemCommand`.
        - The `Menu` class has a `Select` method that is called when an item is selected. We will need to connect this to our cloud-saving logic.
        - While we can create menus programmatically using the `Menu` class, the application's menus are typically defined in XML files in the `uiconfig` directory of each module. The `framework` module is responsible for parsing these files and creating the menus. Therefore, the best approach is to add our new menu items to the relevant `uiconfig` file.
        - `vcl/source/window/dialog.cxx` defines the `Dialog` class. To create our OAuth login window, we can subclass `Dialog` and add the necessary controls.
        - Dialogs can be executed modally using the `Execute()` method, which returns a result code (e.g., `RET_OK`, `RET_CANCEL`).
        - We will need to investigate how to embed a web view component into our dialog. This might involve using a platform-specific control or a cross-platform library that integrates with VCL. A good starting point would be to search the codebase for existing uses of web views.
- **`framework` (UNO Framework):** This module is responsible for building the toolbars and menus. We will need to interact with it to add our new menu items.
    - **Deeper Dive:** `framework/source/uiconfiguration/uiconfigurationmanager.cxx` implements the `UIConfigurationManager` service, which is responsible for managing all aspects of the user interface configuration.
        - This service reads UI definitions from XML files located in the `uiconfig` directory of each module.
        - The `UIConfigurationManager` aggregates the configuration settings from different modules to build the final UI.
        - To add our "Save to Cloud" menu item, we will need to find the correct `uiconfig` file for the "File" menu (likely in the `sfx2` or `sw` module) and add a new entry for our item. The entry will specify the item's label, command URL, and position in the menu.
        - The command URL is a UNO-style URL that is used to dispatch the command when the menu item is selected. We will need to implement a UNO component that handles this command and triggers our cloud-saving logic.
- **`desktop`:** This module contains the application's entry point (`soffice_main`). While we may not need to modify it directly, understanding the application's startup sequence will be important for initializing our authentication service and any other cloud-related components.

### Key Interfaces:
- **`com.sun.star.embed.XStorage`:** This interface provides access to the document's storage. We will need to implement this interface to create a custom storage that reads from and writes to S3.
- **`com.sun.star.ucb.XContentProvider`:** If we choose to implement a UCB provider for our cloud service, we will need to implement this interface.
- **`com.sun.star.ui.XContextMenuInterceptor`:** We will need to implement this interface to add our new menu items to the "File" menu.
- **`com.sun.star.awt.XWindow`:** The base interface for all UI components. We will use this to create our OAuth login dialog.

### Conventions and Considerations:
- **UNO (Universal Network Objects):** The codebase makes extensive use of UNO, a component model similar to COM. New features, especially those that need to be accessible from different parts of the application (like an authentication service), should be implemented as UNO components.
- **`SolarMutex`:** This is a global lock that must be used when accessing shared resources. Given that our cloud features will involve asynchronous operations, we need to be extremely careful with locking to avoid deadlocks and race conditions. The `vcl/README.md` provides some guidance on this.

## 2. Mobile-Responsive Web App

This feature is largely independent of the core LibreOffice codebase. However, the web app will need to interact with the same cloud storage backend (S3) and authentication service (e.g., Cognito) as the desktop application. The core LibreOffice codebase will not be directly affected, but we will need to ensure that the file formats we use are compatible with both the desktop and web versions.

## 3. Smart Rewrite with AI

### Relevant Modules:
- **`sw` (Writer):** This is the most relevant module for this feature.
    - The `sw/README.md` provides a detailed overview of the Writer core. The `SwDoc` class represents the document, and the `SwNodes` array contains the document's content.
    - We will need to interact with the text attributes (`SwTextAttr`) to identify the selected text and replace it with the rewritten version.
    - The Undo/Redo mechanism (`sw::UndoManager`) must be properly handled to ensure that the rewrite operation can be undone.
    - **Deeper Dive:** `sw/inc/doc.hxx` is the header file for the `SwDoc` class.
        - `SwDoc` provides access to the document's content through the `GetNodes()` method, which returns a `SwNodes` object.
        - To get the selected text, we will need to work with a `SwPaM` (Pointer and Mark), which represents a selection. The `SwPaM` can be used to iterate over the `SwNodes` within the selection.
        - The `IDocumentContentOperations` interface, accessible via `getIDocumentContentOperations()`, provides methods for inserting, deleting, and modifying content. We will use these methods to replace the selected text with the AI-rewritten version.
        - The `GetIDocumentUndoRedo()` method returns an `IDocumentUndoRedo` interface, which we can use to add our rewrite operation to the undo stack. We should create a custom undo action that stores the original text, so that the user can revert the change.
- **`vcl` (Visual Class Library):** We will need to add a new context menu item ("Smart Rewrite") that appears when text is selected.

### Key Interfaces:
- **`com.sun.star.text.XTextDocument`:** The main interface for a text document. We will use this to get the selected text and to insert the rewritten text.
- **`com.sun.star.text.XTextRange`:** Represents a range of text in a document. We will use this to get the content of the selection.
- **`com.sun.star.util.XUndoManager`:** We will need to interact with this interface to add our rewrite operation to the undo stack.
- **`com.sun.star.ui.XContextMenuInterceptor`:** We will need to implement this interface to add our "Smart Rewrite" item to the context menu.

### Conventions and Considerations:
- **Extending the UI:** Adding a new context menu item will require modifying the UI configuration, likely in the `uiconfig` directory of the `sw` module.
- **External API Integration:** This feature will involve calling an external AI service. We need to ensure that this is done asynchronously to avoid blocking the UI thread. We should also consider how to handle API keys and other sensitive information securely.

## 4. Easy Add-Font Feature

### Relevant Modules:
- **`vcl` (Visual Class Library):** This module handles font management.
    - The `vcl/README.md` mentions that each platform backend has its own `SalData` implementation, which is responsible for font handling. We will need to investigate how to add a new font at runtime on each platform (Windows, macOS, Linux).
    - We will need to create a new dialog that allows the user to select a TTF file.
    - **Deeper Dive:** `vcl/source/font/font.cxx` defines the `Font` class, which encapsulates the properties of a font (family, style, size, etc.).
        - The `Font` class itself does not seem to handle the loading of font files from disk. Instead, it appears to be a wrapper around a lower-level, platform-specific font representation.
        - A key function, `identifyTrueTypeFont`, takes a memory buffer containing TTF data and extracts font properties. This suggests that there is a mechanism for loading font files into memory, which we can leverage for our feature.
        - **Linux:** The font management on Linux is handled by the `FontManager` class in `vcl/unx/generic/fontmanager/fontmanager.cxx`. This class is responsible for finding, adding, and analyzing font files. We can use the `AddTempDevFont` method to add a new font at runtime.
        - **Windows:** The font management on Windows is handled by the `SalFont` class in `vcl/win/gdi/salfont.cxx`. This class uses the `AddFontResourceExW` Windows API function to add a font from a file. We will need to call this function with the path to the user's TTF file.
        - **macOS:** The font management on macOS is more difficult to trace. The standard macOS font management APIs (`CTFontManagerRegisterFontsForURL` and `ATSFontActivateFromFileSpecification`) are not used in the codebase. It is likely that the macOS implementation uses a more abstract, cross-platform mechanism that is not immediately obvious from the code. Further investigation will be needed to determine the correct way to add a font at runtime on macOS.
- **`sfx2` (Legacy Framework):** This module contains the `SfxStyleSheetPool`, which manages the document's styles. When a new font is added, we will need to update the style sheets to make the font available in the UI.

### Key Interfaces:
- **`com.sun.star.awt.XToolkit`:** This interface provides access to the platform's windowing toolkit. We will use this to create the file selection dialog.
- **`com.sun.star.graphic.XGraphicProvider`:** This interface is used to load and save graphics, but it also has some font-related functionality that we may need to use.
- **`com.sun.star.style.XStyleFamiliesSupplier`:** We will need to use this interface to access the document's style families and add the new font to the font list.

### Conventions and Considerations:
- **Platform-Specific Code:** This feature will likely require writing platform-specific code for each operating system.
- **Font Caching:** We need to understand how LibreOffice caches fonts to ensure that newly added fonts are immediately available.

## 5. Draggable and Playable MP3 Files

### Relevant Modules:
- **`sd` (Draw/Impress):** This module has the most sophisticated handling of embedded objects.
    - The `sd/README.md` mentions `drawingml` as the component responsible for handling shapes. We will likely need to create a new type of shape or embedded object to represent the MP3 player.
- **`avmedia`:** This module likely contains the code for handling audio and video playback. We will need to investigate how to use it to play the embedded MP3 file.
- **`vcl` (Visual Class Library):** This module handles drag and drop.
    - The drag and drop mechanism is platform-specific. On Windows, it is implemented using the `IDropTarget` COM interface in `vcl/win/dtrans/idroptarget.cxx`. This class delegates the work to a `DropTarget` class.
    - **Deeper Dive:** The `DropTarget` class, defined in `vcl/inc/win/dnd_target.hxx`, is a C++ implementation of the `XDropTarget` UNO interface.
        - The `DropTarget::Drop` method is the entry point for handling a drop event. It receives a `DropTargetEvent` object that contains information about the dropped data, including the data's format and the drop location.
        - To implement our feature, we will need to create a new `DropTargetListener` and register it with the `DropTarget`. The listener will need to check if the dropped data is a file and if that file is an MP3.
        - If the dropped file is an MP3, the listener will need to create a new media object. The `avmedia` module, which handles audio and video playback, is likely the correct place to look for a suitable media object implementation. We will need to investigate how to create a media object and insert it into the document at the drop location.
        - The `slideshow` module might also contain relevant code for handling media objects, as it is responsible for displaying presentations with embedded media.

### Key Interfaces:
- **`com.sun.star.datatransfer.dnd.XDropTargetListener`:** We will need to implement this interface to handle the drop event.
- **`com.sun.star.media.XPlayer`:** This interface provides the basic controls for playing media files. We will need to use this interface to create a custom media player for the embedded MP3 files.
- **`com.sun.star.drawing.XShape`:** The base interface for all shapes. We will likely need to create a custom shape that represents the MP3 player.

### Conventions and Considerations:
- **Object Model:** We need to decide how to represent the MP3 file in the document's object model. It could be a special type of frame or a custom shape.
- **Filters:** We will need to modify the ODF import/export filters (in `xmloff`) to ensure that the embedded MP3 files are saved and loaded correctly. We may also need to consider how this feature interacts with other file formats, like DOCX and PPTX. 

## 6. General Conventions and Patterns

- **UNO (Universal Network Objects):** This is the core component model of LibreOffice. Most of the application's functionality is exposed as UNO services. We will need to interact with existing UNO services and may need to create our own to implement the new features.
- **XML Configuration:** The UI and other aspects of the application are configured using XML files. We will need to modify these files to add our new UI elements.
- **Platform Abstraction:** The `vcl` module provides a platform abstraction layer, but some functionality is still platform-specific. We will need to be mindful of this when implementing features that interact with the operating system, such as font management and drag and drop.
- **Backwards Compatibility:** We must ensure that our changes do not break existing functionality. This means we need to be careful when modifying core components and should add new functionality in a way that is as isolated as possible. 

## 7. The UNO Component Model

UNO (Universal Network Objects) is the component model used throughout the LibreOffice codebase. It is a language-independent and platform-independent framework that allows components written in different programming languages to interact with each other. Understanding UNO is essential for developing new features for LibreOffice.

### What is UNO?

At its core, UNO is a system for defining and implementing interfaces. An interface is a collection of methods that define a specific functionality. For example, the `com.sun.star.frame.XComponentLoader` interface has a `loadComponentFromUrl` method that is used to load documents.

UNO components are objects that implement one or more interfaces. These components can be written in any language that has a UNO language binding, such as C++, Java, or Python.

### How does UNO work?

UNO uses an Interface Definition Language (IDL) to define its interfaces and services. The IDL is a language-neutral way of describing the methods, properties, and other elements of a component. The IDL files are compiled into type libraries (`.rdb` files) that are used by the UNO runtime to manage the components.

When a component is instantiated, the UNO runtime creates a proxy object that the client code interacts with. This proxy object handles the communication with the actual component, which may be in a different process or even on a different machine. This is how UNO achieves its language and platform independence.

### Key Concepts

- **Interfaces:** The fundamental building blocks of UNO. They define a contract between the client and the component. All interfaces inherit from the base interface `com.sun.star.uno.XInterface`.
- **Services:** A service is a specification of a component's functionality. It defines the interfaces that a component must implement to provide a particular service. Services are the primary way that components are instantiated in UNO.
- **Components:** The actual implementations of services. They are typically packaged as shared libraries (`.so` or `.dll` files) or Java archives (`.jar` files).
- **Service Manager:** The central factory for creating instances of services. The service manager is responsible for finding and loading the components that implement the requested services.
- **Component Context:** The environment in which a component lives. The component context provides access to the service manager and other resources.

### Design Considerations when using UNO

- **Interfaces are contracts:** When you implement a UNO interface, you are agreeing to abide by the contract defined by that interface. This means that you must implement all of the methods of the interface, and you must do so in a way that is consistent with the documentation.
- **Use services to instantiate components:** The service manager is the preferred way to create instances of components. This allows the application to be more flexible and extensible, as new implementations of a service can be added without changing the client code.
- **Be mindful of threading:** UNO is a multi-threaded environment, and you need to be careful to avoid race conditions and deadlocks. The `SolarMutex` is a global lock that should be used when accessing shared resources.
- **Handle exceptions:** UNO methods can throw exceptions to indicate errors. You should always be prepared to catch and handle these exceptions in your code.
- **Manage object lifetimes:** UNO uses a reference counting mechanism to manage the lifetime of its objects. The `acquire()` and `release()` methods of the `XInterface` interface are used to increment and decrement the reference count of an object. In most language bindings, this is handled automatically, but it is still important to be aware of the underlying mechanism. 