// imgui_kit's touch demo on iOS: Dear ImGui's Apple Metal example (its iOS part), plus the kit's touch layer.
// https://github.com/pthom/imgui_kit (MIT license)
//
// What the kit adds to Dear ImGui's example: the lines marked "imgui_kit". The rest is what any iOS app needs:
// all the fingers reported (multipleTouchEnabled), the safe area (the notch), the keyboard (UIKeyInput).

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_kit.h"
#include <float.h>

@interface AppViewController : UIViewController <MTKViewDelegate, UIKeyInput>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, weak) UITouch* mouseTouch; // the first finger: Dear ImGui's mouse
@end

@implementation AppViewController

-(instancetype)initWithNibName:(nullable NSString*)nibNameOrNil bundle:(nullable NSBundle*)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // the app's folder is read-only
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().FontSizeBase = 17.0f;  // iOS's body text size, in points
    ImGui_ImplMetal_Init(_device);

    ImGuiTouch::Setup();            // imgui_kit: the touch layer (swipe, long press, pinch)
    ImGuiTouch::SetupWheelSession();  // imgui_kit: the wheel (a trackpad or a mouse on an iPad)

    // Dear ImGui's links (TextLinkOpenURL) open in Safari
    ImGui::GetPlatformIO().Platform_OpenInShellFn = [](ImGuiContext*, const char* url) {
        NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
        if (nsUrl == nil)
            return false;
        [[UIApplication sharedApplication] openURL:nsUrl options:@{} completionHandler:nil];
        return true;
    };
    return self;
}

-(void)loadView
{
    self.view = [[MTKView alloc] initWithFrame:CGRectZero];  // the window sizes it
}

-(void)viewDidLoad
{
    [super viewDidLoad];
    MTKView* view = (MTKView*)self.view;
    view.device = self.device;
    view.delegate = self;
    view.multipleTouchEnabled = YES;  // all the fingers, for the pinch and the two-finger drag
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillChange:)
                                                 name:UIKeyboardWillChangeFrameNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide:)
                                                 name:UIKeyboardWillHideNotification object:nil];
}

-(void)keyboardWillChange:(NSNotification*)notification
{
    CGRect frame = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect inView = [self.view convertRect:frame fromView:nil];
    float hidden = (float)MAX(0.0, CGRectGetMaxY(self.view.bounds) - CGRectGetMinY(inView));
    ImGuiTouch::SetKeyboardInsets(ImGuiTouchKeyboardInsets{0.0f, hidden});  // imgui_kit: the caret stays above it
}

-(void)keyboardWillHide:(NSNotification*)notification
{
    ImGuiTouch::SetKeyboardInsets(ImGuiTouchKeyboardInsets{});  // imgui_kit
}

-(void)drawInMTKView:(MTKView*)view
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)view.bounds.size.width, (float)view.bounds.size.height);
    CGFloat scale = view.window.screen.scale ?: UIScreen.mainScreen.scale;
    io.DisplayFramebufferScale = ImVec2((float)scale, (float)scale);

    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    if (renderPassDescriptor == nil)
    {
        [commandBuffer commit];
        return;
    }
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui::NewFrame();

    // The kit's demo, in a window that fills the screen, out of the notch and the keyboard
    UIEdgeInsets safe = view.safeAreaInsets;
    float bottom = MAX((float)safe.bottom, ImGuiTouch::GetKeyboardInsets().Bottom);  // imgui_kit
    ImGui::SetNextWindowPos(ImVec2((float)safe.left, (float)safe.top));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - (float)(safe.left + safe.right),
                                    io.DisplaySize.y - (float)safe.top - bottom));
    ImGui::Begin("imgui_kit touch demo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextWrapped("Open \"Touch\" below, then try it with a finger.");
    ImGui::TextLinkOpenURL("See the full interactive explorer", "https://pthom.github.io/imgui_explorer/?lib=imguikit");
    ImGuiKit::ShowDemoContents();
    ImGui::End();

    ImGui::Render();
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0);
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);
    [renderEncoder endEncoding];
    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];

    // The keyboard follows Dear ImGui's wish for text
    if (io.WantTextInput && !self.isFirstResponder)
        [self becomeFirstResponder];
    else if (!io.WantTextInput && self.isFirstResponder)
        [self resignFirstResponder];
}

-(void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size
{
}

// The fingers: each one goes to the kit (the pinch, two fingers dragged together); the first one is also Dear
// ImGui's mouse. Between two touches there is no pointer (nothing stays hovered).
-(void)updateTouches:(UIEvent*)event
{
    ImGuiIO& io = ImGui::GetIO();
    for (UITouch* touch in event.allTouches)
    {
        CGPoint p = [touch locationInView:self.view];
        bool down = touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled;
        int id = (int)(touch.hash & 0x7fffffff);  // stable while the finger is down
        ImGuiTouch::AddFingerEvent(id, ImVec2((float)p.x, (float)p.y), down);  // imgui_kit

        if (self.mouseTouch == nil && down)
            self.mouseTouch = touch;
        if (touch != self.mouseTouch)
            continue;
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent((float)p.x, (float)p.y);
        io.AddMouseButtonEvent(0, down);
        if (!down)
        {
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            self.mouseTouch = nil;
        }
    }
}

-(void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event     { [self updateTouches:event]; }
-(void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event     { [self updateTouches:event]; }
-(void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event     { [self updateTouches:event]; }
-(void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self updateTouches:event]; }

// The keyboard (UIKeyInput): what is typed goes to Dear ImGui
-(BOOL)canBecomeFirstResponder { return YES; }
-(BOOL)hasText { return YES; }
-(void)insertText:(NSString*)text
{
    ImGuiIO& io = ImGui::GetIO();
    if ([text isEqualToString:@"\n"])
    {
        io.AddKeyEvent(ImGuiKey_Enter, true);
        io.AddKeyEvent(ImGuiKey_Enter, false);
    }
    else
        io.AddInputCharactersUTF8(text.UTF8String);
}
-(void)deleteBackward
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_Backspace, true);
    io.AddKeyEvent(ImGuiKey_Backspace, false);
}
-(UITextAutocorrectionType)autocorrectionType { return UITextAutocorrectionTypeNo; }
-(UITextAutocapitalizationType)autocapitalizationType { return UITextAutocapitalizationTypeNone; }

@end


// The scene lifecycle (required since iOS 27): the scene creates the window (Info.plist names this class)
@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation SceneDelegate
-(void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options
{
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
    self.window.rootViewController = [[AppViewController alloc] initWithNibName:nil bundle:nil];
    [self.window makeKeyAndVisible];
}
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation AppDelegate
-(BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    return YES;
}
@end

int main(int argc, char* argv[])
{
    @autoreleasepool
    {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
