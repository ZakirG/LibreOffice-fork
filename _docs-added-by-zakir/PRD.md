# LibreOffice Fork, by Zakir

We have forked LibreOffice locally. We will be adding new features to it and refactoring the code as needed. But for the most part, we want to respect the conventions set by the codebase. And we must preserve all existing features without causing any regressions.

Objectives: 
- Turn LibreOffice into a cloud-native collaborative editing platform like Google Docs, but built on AWS, and mobile-friendly. 
- Implement the 6 new features listed below.
- Make sense of the codebase and document how we implement our changes.
- Make our changes in a way that respects the existing structure of the codebase (described below in the section marked "codebase overview").
- Optional: Make the UI beautiful and modern. This will require us to properly understand how the UI is built and rendered.

Target niche:
- Musicians and mix engineers. There's just two main features related to musicians, which is the feature where you can drag in an audio file and listen to it in the document and the feature where you can edit documents on your phone via the cloud storage and a mobile app.

LibreOffice Fork

Six new features:
1) Modern authentication: a login feature with Google OAuth support. This will be enable the Save to Cloud feature. The same authentication that powers our LibreOffice cloud access will power the mobile-friendly webapp, allowing for easy document syncing across devices.
2) File -> Save to Cloud. New option in File Menu. When the user clicks this, we save their file to AWS s3 so that the documents are accessible from anywhere. They can also open documents saved in the cloud while in LibreOffice. We fetch documents from the user's folder in the s3 bucket and allow the user to open them and continue editing while in LibreOffice.
3) Cloud files view in LibreOffice. List all cloud-saved files from the user's folder in the s3 bucket.
4) Mobile-responsive web-app to view documents saved to cloud
5) Select text -> Smart rewrite with AI. Easily fixes all grammar, spelling, capitalization, punctuation.
6) Drag in mp3 file that is playable within a document. Right now when you drag in an mp3, you just see a square image of a music note. No play button. I want to make that mp3 easily playable in the UI, easy play button. The user should be able to listen to the file and also write mix notes right underneath it.


# Codebase Details for New Features

This section outlines the key areas of the LibreOffice codebase that will be relevant for implementing the new features described in the PRD. 

## 1 & 2. Cloud Storage and Authentication

### Architecture Overview

The desktop application will integrate with a companion Next.js web application that serves as a broker for authentication and file storage. The C++ client will not directly handle OAuth or interact with the AWS SDK. Instead, it will function as a standard HTTP client communicating with a set of well-defined REST API endpoints provided by the web app.

This approach centralizes complex logic (authentication, session management, file metadata, S3 communication) in the web application, leaving the desktop client with three primary responsibilities:
1.  Initiating a browser-based authentication flow.
2.  Making API calls with a JWT to get pre-signed S3 URLs.
3.  Performing direct HTTP `PUT` (for uploads) and `GET` (for downloads) requests to the provided S3 URLs.

### Adding the "Login to Cloud" Menu Item

To provide a user entry point for authentication, a "Login to Cloud" item will be added to the "File" menu.

#### 1. UI Configuration

A new menu item will be defined in the relevant UI configuration files. The most important of these is `sw/uiconfig/swriter/menubar/menubar.xml`, which controls the main menu bar for the Writer application. A new line will be added:

```xml
<menu:menuitem menu:id=".uno:LoginToCloud"/>
```

This will be placed logically alongside existing remote-related items like `.uno:SaveAsRemote`. The `menu:id` defines a new UNO command that the framework must learn to handle.

#### 2. Command Definition and Dispatch

The `.uno:LoginToCloud` command must be mapped to a specific action. This involves two steps:

a. **Slot Definition:** A new "slot" will be defined. Slots are the internal mechanism that connects UI commands to application logic. This definition will be added in a relevant part of the `sfx2` module, likely by creating a new slot ID (e.g., `SID_LOGIN_TO_CLOUD`) and associating it with the UNO command string `.uno:LoginToCloud`.

b. **Dispatch Implementation:** The framework's dispatch mechanism will be leveraged to execute our code when the menu item is clicked. The `SfxAppDispatchProvider` class in `sfx2/source/appl/appdispatchprovider.cxx` is responsible for querying for a dispatcher for a given UNO command. It creates an `SfxOfficeDispatch` object (`sfx2/source/control/unoctitm.cxx`) which gets associated with our new slot. We will implement the logic inside the handler for this new slot, which will be a new C++ class responsible for initiating the authentication flow.

### C++ Implementation Details

#### Module Responsibilities

- **`sfx2` (Application Framework):** This module will contain most of the new client-side logic.
    - **Authentication Orchestration:** A new class, e.g., `CloudAuthHandler`, will be created to manage the entire authentication sequence. It will be implemented as a UNO component that handles the `.uno:LoginToCloud` command.
    - **HTTP Communication:** A new helper class, e.g., `CloudApiClient`, will be implemented to handle all HTTP communication with the web API.
    - **File I/O Interception:** The `SfxMedium` class (`sfx2/source/doc/docfile.cxx`) will be modified or hooked. When a document is saved to a special URL scheme (e.g., `cloud://<doc_id>`), the standard file-write operation will be bypassed, and our `CloudApiClient` will be used to perform the cloud upload.

- **`ucb` (Universal Content Broker):** The existing HTTP client infrastructure will be reused.
    - The `CurlSession` class (`ucb/source/ucp/webdav-curl/CurlSession.cxx`), a wrapper around `libcurl`, will be instantiated by our `CloudApiClient` to perform `GET`, `POST`, and `PUT` requests. This provides a robust, existing solution for raw HTTP communication.

- **`vcl` (Visual Class Library):** Provides the UI components for user interaction.
    - A simple, non-modal dialog will be created using `vcl` widgets to inform the user that they need to complete the login in their browser. This dialog will be displayed by the `CloudAuthHandler`.

- **`framework`:** This module's UI configuration manager (`framework/source/uiconfiguration/uiconfigurationmanager.cxx`) will read our `menubar.xml` changes and render the new menu item. No code changes are needed here; we are just using its existing functionality.

#### Authentication Flow Implementation

1.  The user clicks "File" -> "Login to Cloud".
2.  The `framework` dispatches the `.uno:LoginToCloud` command to our new `CloudAuthHandler` component in `sfx2`.
3.  The handler uses the `CloudApiClient` to send a `POST` request to the `/api/desktop/init-login` endpoint of our web app.
4.  The API returns a JSON object containing a unique login URL.
5.  The handler uses a platform-agnostic function from the `sal` (System Abstraction Layer) to open the returned URL in the user's default external web browser.
6.  The handler displays a small, non-blocking `vcl` dialog saying "Please complete your login in the browser."
7.  Simultaneously, the handler starts polling the `/api/desktop/token?nonce=...` endpoint every few seconds in a background thread. This is critical to avoid freezing the application UI. The `SolarMutex` must be handled correctly for any UI updates from this background thread.
8.  Once the user successfully logs in on the web, the polling request receives a JSON Web Token (JWT).
9.  The JWT is securely stored in the user's LibreOffice configuration profile using the `officecfg` component. The `vcl` dialog is closed, and the UI may be updated to show a "logged in" state.

#### Save/Refresh Flow Implementation

1.  **Save to Cloud:** When the user invokes "Save to Cloud", our modified `SfxMedium` logic intercepts the call. It uses the `CloudApiClient` to send the stored JWT in an `Authorization: Bearer` header to the `/api/docs/presign?mode=put` endpoint. The API returns a pre-signed S3 `PUT` URL. The `CloudApiClient` then uses `CurlSession` to `PUT` the document's byte stream directly to that S3 URL.
2.  **Refresh from Cloud:** A similar flow occurs. The client calls `/api/docs/presign?mode=get` and receives a pre-signed `GET` URL, from which it downloads the latest version of the document.

## 3. Mobile-Responsive Web App

This feature is **central to the new architecture**. It is not just a viewer but also the **backend API server** for the desktop application. Built with Next.js and deployed on Vercel, it will:
- Handle all user authentication (using NextAuth).
- Manage user and document metadata in a Postgres database (via Prisma).
- Implement the API endpoints that the desktop client will use for authentication and to obtain pre-signed S3 URLs.
- Provide the mobile-friendly web dashboard for users to browse and manage their documents.
See this file for more details: webapp-for-auth-and-cloud-docs.md

## 4. Smart Rewrite with AI

### Architecture Overview

This feature will integrate an external AI text-generation service to provide users with intelligent text-rewriting capabilities. The user will select text, right-click to open a context menu, and choose the "Smart Rewrite with AI" option. This will trigger a custom dialog where the user can select a rewrite style or provide a custom prompt. The desktop client will then make an asynchronous HTTP request to an external API, receive the rewritten text, and replace the user's selection.

### Adding the "Smart Rewrite" Context Menu Item

The entry point for this feature will be a new item in the context menu that appears when text is selected.

#### 1. Context Menu Interception

The context menu must be modified at runtime. This will be achieved by implementing and registering a `com.sun.star.frame.XDispatchProviderInterceptor`.

a. **Interceptor Implementation:** A new class, `SmartRewriteInterceptor`, will be created in the `sw` module. It will implement the `css::frame::XDispatchProviderInterceptor` interface. This pattern can be seen in `sw/source/uibase/uno/unodispatch.cxx` (`SwXDispatchProviderInterceptor`).

b. **Registration:** An instance of `SmartRewriteInterceptor` will be registered with the frame's `XDispatchProviderInterception` interface, which is available on the `SwView`'s frame. This gives our interceptor the first opportunity to handle context menu-related dispatches.

c. **Action Handling:** The interceptor will specifically watch for the context menu command. When it detects that the context menu is about to be shown over a text selection, it will dynamically add a new menu item labeled "Smart Rewrite with AI" and associate it with a new custom UNO command, `.uno:SmartRewrite`.

#### 2. Creating the Rewrite Dialog

When the user clicks the ".uno:SmartRewrite" menu item, a new dialog will be displayed.

a. **Dialog Definition:** The dialog's layout will be defined in a new UI file (e.g., `smart_rewrite_dialog.ui`) within the `sw/uiconfig/` directory. This approach follows the pattern seen in `sw/source/ui/fldui/DropDownFieldDialog.cxx` and other modern dialog implementations.

b. **VCL Components:** The dialog will be constructed using the `vcl` (Visual Component Library) and the `weld` UI builder abstraction. It will contain:
    - A `ComboBox` (`vcl/source/control/combobox.cxx`) for pre-defined rewrite styles ("Professional", "Concise", "Friendly", "Polite").
    - A multi-line `Edit` field (`include/vcl/toolkit/edit.hxx`) for the user to enter a custom rewrite prompt (e.g., "Make this sound more confident").
    - "OK" and "Cancel" buttons.

c. **Dialog Controller:** A new controller class, `SmartRewriteDialogController`, will be created to manage the dialog's logic. It will be responsible for populating the `ComboBox`, capturing the user's input, and executing the rewrite action when "OK" is clicked.

### C++ Implementation Details

#### Module Responsibilities

- **`sw` (Writer Module):** This module will contain all the logic for this feature.
    - **`SmartRewriteInterceptor`:** As described above, this class will handle the context menu modification.
    - **`SmartRewriteDialogController`:** This class will manage the UI dialog.
    - **`SmartRewriteHandler`:** A new class to orchestrate the entire process. It will be triggered by the `.uno:SmartRewrite` command, get the currently selected text from the `SwPaM` (Pointer and Mark) in the `SwDoc`, display the `SmartRewriteDialogController`, and then execute the API call.

#### Text and API Flow

1.  The `SmartRewriteHandler` obtains the selected text as a string from the current `SwPaM` (Pointer and Mark) in the `SwDoc`.
2.  When the user clicks "OK" in the dialog, the handler retrieves the chosen style from the `ComboBox` and the custom prompt from the `Edit` field.
3.  It constructs a JSON payload for the external AI API, including the selected text and the rewrite instructions.
4.  Using the same `CloudApiClient` and `CurlSession` infrastructure from the cloud storage feature, it makes an asynchronous `POST` request to the AI service's endpoint. This must happen on a background thread to avoid locking the UI.
5.  Upon receiving a successful JSON response containing the rewritten text, the handler dispatches a task back to the main UI thread.
6.  This task will use the `IDocumentContentOperations` interface (retrieved via `GetDoc()->getIDocumentContentOperations()`) to replace the currently selected text with the new text from the API.
7.  Crucially, this entire operation will be wrapped in a single `Undo` action. The `GetIDocumentUndoRedo()->AddUndo(..)` method will be used to create a custom undo object that stores the original text, allowing the user to revert the change with a single Ctrl+Z.

### Conventions and Considerations

- **API Key Management:** API keys for the external AI service must not be hardcoded. They will be stored securely and loaded at runtime, likely from the user's configuration profile managed by `officecfg`.
- **Asynchronous Operations:** All network requests must be asynchronous to ensure the UI remains responsive. The `SolarMutex` must be managed correctly when communicating between the background network thread and the main UI thread.

## 5. Easy Add-Font Feature

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

## 6. Playable Audio Files

We already have the functionality to drag in an audio file into the document and see a media representation of it. But there is no filename display and the media is not playable. We'll be changing that.
When a user drags in an audio file (e.g., `.mp3`, `.wav`, `.m4a`) into a text document, the displayed preview should have a play/pause button that plays the audio file when clicked.
In addition, instead of the media preview being a square (it is currently a square with a big musical note icon in the middle) it should be a wide rectangle with a play/pause button in the middle of it and a filename string underneath that button. we will remove the current musical note icon from the preview.

### Technical Implementation Plan

Our implementation will focus on replacing the default static media object preview with a custom, interactive control for audio files. This involves creating a new UI control for the player and modifying the rendering pipeline to use it for supported audio formats (`.mp3`, `.wav`, `.m4a`).

#### 1. Create a Custom `AudioPlayerControl`

A new class, `AudioPlayerControl`, will be created within the `vcl` module. This class will inherit from `vcl::Control` and will be responsible for the entire visual representation and functionality of the audio player.

- **`vcl/source/control/audioplayer.cxx` and `vcl/inc/audioplayer.hxx`:** New files will be created to house the implementation of `AudioPlayerControl`.
- **UI Components:** This control will manage:
    - A play/pause button.
    - A text label to display the audio filename.
- **Playback Logic:** The `AudioPlayerControl` will use the `avmedia::Player` class from the `avmedia` module to handle the actual audio playback. It will be responsible for loading the audio file from its URL and managing play/pause states.
- **Event Handling:** The control will implement `MouseButtonDown` event handlers to toggle playback when the user clicks the play/pause button.

#### 2. Modify `MediaPrimitive2D` to Use the New Control

The `MediaPrimitive2D` is responsible for rendering the generic media object. We will modify its `create2DDecomposition` method to special-case audio files.

- **`core/drawinglayer/source/primitive2d/mediaprimitive2d.cxx`:**
    - In the `create2DDecomposition` method, we will add a check for the media's file extension (`.mp3`, `.wav`, `.m4a`).
    - If the media is identified as a supported audio file, instead of creating a `GraphicPrimitive2D` for the default icon, the code will instantiate our new `AudioPlayerControl`.
    - The existing logic for drawing the background and border will be preserved.

#### 3. Enhance `ViewContactOfSdrMediaObj` to Pass Data

The `ViewContactOfSdrMediaObj` class needs to be updated to provide the necessary information (like the filename) to the rendering layer.

- **`core/svx/source/sdr/contact/viewcontactofsdrmediaobj.cxx`:**
    - The `createViewIndependentPrimitive2DSequence` method will be modified.
    - It will extract the filename from the `SdrMediaObj`'s URL.
    - This filename string will be passed down to the `MediaPrimitive2D` and subsequently to the `AudioPlayerControl` for display.

#### 4. Reshaping the Media Object

The current media object is a square. We will modify the object's geometry when it's identified as a supported audio file.

- **`core/svx/source/svdraw/svdomedia.cxx`:** When a supported audio file is dropped, we will need to adjust the default `SdrMediaObj`'s `SetGeoRect` to create a wide rectangle instead of a square, to better accommodate the player UI.

This approach ensures that our changes are well-encapsulated. We create a new, self-contained UI component for the audio player and then selectively use it within the existing rendering framework without disrupting the handling of other media types.

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

