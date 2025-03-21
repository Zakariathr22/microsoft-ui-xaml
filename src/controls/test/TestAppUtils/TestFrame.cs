﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControls.TestAppUtils;
using System;
using System.Threading;
using Windows.Foundation;
using Windows.Graphics.Display;
using Windows.System.Threading;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using System.Runtime.InteropServices;
using Windows.UI.Core;

namespace MUXControlsTestApp
{
    public sealed partial class TestFrame : Frame
    {
        public UIElement CustomElement
        {
            get { return (UIElement)GetValue(CustomElementProperty); }
            set { SetValue(CustomElementProperty, value); }
        }

        public static DependencyProperty CustomElementProperty = DependencyProperty.Register("CustomElement", typeof(UIElement), typeof(TestFrame), null);
        private Viewbox _rootViewbox = null;
        private Grid _rootGrid = null;
        private Button _backButton = null;
        private Button _goBackInvokerButton = null;
        private Button _goFullScreenInvokerButton = null;
        private Button _toggleThemeButton = null;
        private ToggleButton _innerFrameInLabDimensions = null;
        private ToggleButton _enableRTL = null;
        private TextBlock _currentPageTextBlock = null;
        private readonly Type _mainPageType = null;
        private ContentPresenter _pagePresenter = null;
        private CheckBox _keyInputReceived = null;
        public Grid BackupThemeBackground { get; private set; }

        public TestFrame(Type mainPageType)
        {
            _mainPageType = mainPageType;
            this.DefaultStyleKey = typeof(TestFrame);

            AddHandler(FrameworkElement.KeyDownEvent, new KeyEventHandler(TestFrame_KeyDown) , true);
        }

        private void TestFrame_KeyDown(object sender, Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e)
        {
            if(_keyInputReceived != null)
            {
                _keyInputReceived.IsChecked = true;
            }
        }

        public void ChangeBarVisibility(Visibility visibility)
        {
            UIElement bar = (UIElement)GetTemplateChild("TestFrameBar");
            if (bar != null)
            {
                bar.Visibility = visibility;
            }
        }

        private void TestFrame_Navigated(object sender, Microsoft.UI.Xaml.Navigation.NavigationEventArgs e)
        {
            if (e.SourcePageType == _mainPageType)
            {
                _backButton.Visibility = Visibility.Collapsed;
                _currentPageTextBlock.Text = "Home";
            }
            else
            {
                _backButton.Visibility = Visibility.Visible;
                _currentPageTextBlock.Text = (e.Parameter is string ? e.Parameter as string : "");
            }

            GarbageCollection_Click(null, null);
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            this.Navigated += TestFrame_Navigated;

            _rootViewbox = (Viewbox)GetTemplateChild("RootViewbox");
            _rootGrid = (Grid)GetTemplateChild("RootGrid");
            BackupThemeBackground = (Grid)GetTemplateChild("BackupThemeBackground");

            // The root grid is in a Viewbox, which lays out its child with infinite
            // available size, so set the grid's dimensions to match the window's.
            // When running tests on phone, these dimensions will end up getting set to
            // match the screen's unscaled dimensions to deal with scale factor differences
            // between devices and provide some consistency.
            SetRootGridSizeFromXamlRootSize();

            XamlRoot.Changed += OnXamlRootChanged;

            // NOTE: The "BackButton" element automatically has a handler hooked up to it by Frame
            // just by being named "BackButton"
            _backButton = (Button)GetTemplateChild("BackButton");

            _toggleThemeButton = (Button)GetTemplateChild("ToggleThemeButton");
            _toggleThemeButton.Click += ToggleThemeButton_Click;

            _pagePresenter = (ContentPresenter)GetTemplateChild("PagePresenter");

            _innerFrameInLabDimensions = (ToggleButton)GetTemplateChild("InnerFrameInLabDimensions");
            _innerFrameInLabDimensions.Checked += _innerFrameInLabDimensions_Checked;
            _innerFrameInLabDimensions.Unchecked += _innerFrameInLabDimensions_Unchecked;
            if(_innerFrameInLabDimensions.IsChecked == true)
            {
                _innerFrameInLabDimensions_Checked(null, null);
            }
            else
            {
                _innerFrameInLabDimensions_Unchecked(null, null);
            }

            _enableRTL = (ToggleButton)GetTemplateChild("EnableRTL");
            _enableRTL.Checked += _enableRTL_Checked;
            _enableRTL.Unchecked += _enableRTL_Unchecked;
            if (_enableRTL.IsChecked == true)
            {
                _enableRTL_Checked(null, null);
            }
            else
            {
                _enableRTL_Unchecked(null, null);
            }

            _goBackInvokerButton = (Button)GetTemplateChild("GoBackInvokerButton");
            _goBackInvokerButton.Click += GoBackInvokerButton_Click;

            _currentPageTextBlock = (TextBlock)GetTemplateChild("CurrentPageTextBlock");
            _currentPageTextBlock.Text = "Home";

            _goFullScreenInvokerButton = (Button)GetTemplateChild("FullScreenInvokerButton");
            _goFullScreenInvokerButton.Click += GoFullScreenInvokeButton_Click;
            _keyInputReceived = (CheckBox)GetTemplateChild("KeyInputReceived");
        }

        private void _innerFrameInLabDimensions_Checked(object sender, RoutedEventArgs e)
        {
            // Enter CI mode
            _pagePresenter.MaxWidth = 1024;
            _pagePresenter.MaxHeight = 664;
        }

        private void _innerFrameInLabDimensions_Unchecked(object sender, RoutedEventArgs e)
        {
            // Leave CI mode
            _pagePresenter.ClearValue(MaxWidthProperty);
            _pagePresenter.ClearValue(MaxHeightProperty);
        }

        private void _enableRTL_Checked(object sender, RoutedEventArgs e)
        {
            _pagePresenter.FlowDirection = FlowDirection.RightToLeft;
        }

        private void _enableRTL_Unchecked(object sender, RoutedEventArgs e)
        {
            _pagePresenter.FlowDirection = FlowDirection.LeftToRight;
        }

        private void ToggleThemeButton_Click(object sender,RoutedEventArgs e)
        {
            var contentAsFrameworkElement = XamlRoot.Content as FrameworkElement;
            if(contentAsFrameworkElement.RequestedTheme == ElementTheme.Default)
            {
                // Convert theme from default to either dark or light based on application requestedtheme
                contentAsFrameworkElement.RequestedTheme = (Application.Current.RequestedTheme == ApplicationTheme.Light) ? ElementTheme.Light : ElementTheme.Dark;
            }
            // Invert theme
            contentAsFrameworkElement.RequestedTheme = (contentAsFrameworkElement.RequestedTheme == ElementTheme.Light) ? ElementTheme.Dark : ElementTheme.Light;
        }

        private void GarbageCollection_Click(object sender, RoutedEventArgs e)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }

        private void GoFullScreenInvokeButton_Click(object sender, RoutedEventArgs e)
        {
            var icon = _goFullScreenInvokerButton.Content as SymbolIcon;
            if (icon.Symbol == Symbol.FullScreen)
            {
                if (CoreWindow.GetForCurrentThread() != null)
                {
                    try
                    {
                        if (Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().TryEnterFullScreenMode())
                        {
                            icon.Symbol = Symbol.BackToWindow;
                        }
                    }
                    catch (COMException)
                    {
                        // ApplicationView.GetForCurrentView() will throw in desktop scenarios.
                    }
                }
            }
            else
            {
                if (CoreWindow.GetForCurrentThread() != null)
                {
                    try
                    {
                        Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().ExitFullScreenMode();
                        icon.Symbol = Symbol.FullScreen;
                    }
                    catch (COMException)
                    {
                        // ApplicationView.GetForCurrentView() will throw in desktop scenarios.
                    }
                }
            }
        }

        private void OnXamlRootChanged(XamlRoot sender, XamlRootChangedEventArgs args)
        {
            SetRootGridSizeFromXamlRootSize();
        }

        private void SetRootGridSizeFromXamlRootSize()
        {
            _rootGrid.Width = _rootGrid.XamlRoot.Size.Width;
            _rootGrid.Height = _rootGrid.XamlRoot.Size.Height;
        }

        public void SetRootGridSizeToCustomSize(int height, int width)
        {
            if (height >= 0 && width >= 0)
            {
                _rootGrid.Width = width;
                _rootGrid.Height = height;
            }
            else
            {
                SetRootGridSizeFromXamlRootSize();
            }
        }

        private void GoBackInvokerButton_Click(object sender, RoutedEventArgs e)
        {
            if (CanGoBack)
            {
                GoBack();
            }
        }
    }
}
