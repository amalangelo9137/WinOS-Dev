using System;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Windows;
using Microsoft.Win32;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace WinOS_Editor
{
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        private AppDocument _document = null!;
        private AppElementDefinition? _selectedElement;
        private AppEventDefinition? _selectedEvent;
        private AppBindingDefinition? _selectedBinding;
        private string _statusMessage = "Ready to author and export the WinOS app package.";

        public MainWindow()
            : this(null, null)
        {
        }

        public MainWindow(AppDocument? initialDocument, string? initialPackagePath)
        {
            InitializeComponent();
            DataContext = this;

            var solutionRoot = ProjectPaths.FindSolutionRoot(AppContext.BaseDirectory);
            LoadDocument(initialDocument ?? AppDocument.CreateDefault(solutionRoot), initialPackagePath);
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        public Array ElementTypes => Enum.GetValues(typeof(AppElementType));

        public Array EventTypes => Enum.GetValues(typeof(AppEventType));

        public Array ActionTypes => Enum.GetValues(typeof(AppActionType));

        public Array PropertyTypes => Enum.GetValues(typeof(AppPropertyType));

        public AppDocument Document
        {
            get => _document;
            set
            {
                if (ReferenceEquals(_document, value))
                {
                    return;
                }

                if (_document != null)
                {
                    _document.Changed -= OnDocumentChanged;
                }

                _document = value;
                _document.Changed += OnDocumentChanged;
                OnPropertyChanged(nameof(Document));
            }
        }

        public AppElementDefinition? SelectedElement
        {
            get => _selectedElement;
            set
            {
                if (ReferenceEquals(_selectedElement, value))
                {
                    return;
                }

                _selectedElement = value;
                SelectedEvent = value?.Events.FirstOrDefault();
                OnPropertyChanged(nameof(SelectedElement));
                OnPropertyChanged(nameof(HasSelectedElement));
                RenderPreview();
            }
        }

        public AppEventDefinition? SelectedEvent
        {
            get => _selectedEvent;
            set
            {
                if (ReferenceEquals(_selectedEvent, value))
                {
                    return;
                }

                _selectedEvent = value;
                SelectedBinding = value?.Bindings.FirstOrDefault();
                OnPropertyChanged(nameof(SelectedEvent));
                OnPropertyChanged(nameof(HasSelectedEvent));
            }
        }

        public AppBindingDefinition? SelectedBinding
        {
            get => _selectedBinding;
            set
            {
                if (ReferenceEquals(_selectedBinding, value))
                {
                    return;
                }

                _selectedBinding = value;
                OnPropertyChanged(nameof(SelectedBinding));
                OnPropertyChanged(nameof(HasSelectedBinding));
            }
        }

        public bool HasSelectedElement => SelectedElement != null;

        public bool HasSelectedEvent => SelectedEvent != null;

        public bool HasSelectedBinding => SelectedBinding != null;

        public string StatusMessage
        {
            get => _statusMessage;
            set
            {
                if (_statusMessage == value)
                {
                    return;
                }

                _statusMessage = value;
                OnPropertyChanged(nameof(StatusMessage));
            }
        }

        private void OnDocumentChanged(object? sender, EventArgs e)
        {
            RenderPreview();
        }

        private void LoadDocument(AppDocument document, string? sourcePackagePath)
        {
            Document = document;
            SelectedElement = Document.Elements.FirstOrDefault();
            RenderPreview();

            if (!string.IsNullOrWhiteSpace(sourcePackagePath))
            {
                StatusMessage = $"Opened {System.IO.Path.GetFileName(sourcePackagePath)} into the editor.";
            }
        }

        private void RenderPreview()
        {
            if (!IsInitialized)
            {
                return;
            }

            PreviewCanvas.Width = Document.CanvasWidth;
            PreviewCanvas.Height = Document.CanvasHeight;
            PreviewCanvas.Children.Clear();

            foreach (var element in Document.GetOrderedElements().Where(item => item.IsVisible))
            {
                var visual = CreateVisual(element);
                if (visual == null)
                {
                    continue;
                }

                var bounds = Document.GetAbsoluteBounds(element);
                System.Windows.Controls.Canvas.SetLeft(visual, bounds.X);
                System.Windows.Controls.Canvas.SetTop(visual, bounds.Y);
                PreviewCanvas.Children.Add(visual);
            }

            if (SelectedElement != null)
            {
                var selectionBounds = Document.GetAbsoluteBounds(SelectedElement);
                var highlight = new Rectangle
                {
                    Width = selectionBounds.Width,
                    Height = selectionBounds.Height,
                    Stroke = new SolidColorBrush(Color.FromRgb(97, 196, 139)),
                    StrokeThickness = 2,
                    StrokeDashArray = new DoubleCollection(new[] { 6.0, 4.0 }),
                    Fill = Brushes.Transparent,
                    IsHitTestVisible = false,
                };
                System.Windows.Controls.Canvas.SetLeft(highlight, selectionBounds.X);
                System.Windows.Controls.Canvas.SetTop(highlight, selectionBounds.Y);
                PreviewCanvas.Children.Add(highlight);
            }
        }

        private FrameworkElement? CreateVisual(AppElementDefinition element)
        {
            switch (element.Type)
            {
                case AppElementType.RootWindow:
                    return CreateBorderVisual(element, 12);
                case AppElementType.Panel:
                    return CreateBorderVisual(element, element.IsDraggableRegion ? 10 : 6);
                case AppElementType.Label:
                    return CreateLabelVisual(element);
                case AppElementType.Button:
                    return CreateButtonVisual(element);
                case AppElementType.Slider:
                    return CreateSliderVisual(element);
                case AppElementType.TextField:
                    return CreateTextFieldVisual(element);
                case AppElementType.Image:
                    return CreateImageVisual(element);
                default:
                    return null;
            }
        }

        private FrameworkElement CreateBorderVisual(AppElementDefinition element, double cornerRadius)
        {
            return new System.Windows.Controls.Border
            {
                Width = element.Width,
                Height = element.Height,
                Background = new SolidColorBrush(ToColor(element.BackgroundColor)),
                BorderBrush = new SolidColorBrush(ToColor(element.BorderColor)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(cornerRadius),
            };
        }

        private FrameworkElement CreateLabelVisual(AppElementDefinition element)
        {
            return new System.Windows.Controls.Border
            {
                Width = element.Width,
                Height = element.Height,
                Background = Brushes.Transparent,
                Child = new System.Windows.Controls.TextBlock
                {
                    Text = element.Text,
                    Foreground = new SolidColorBrush(ToColor(element.ForegroundColor)),
                    VerticalAlignment = VerticalAlignment.Center,
                    TextWrapping = TextWrapping.Wrap,
                },
            };
        }

        private FrameworkElement CreateButtonVisual(AppElementDefinition element)
        {
            return new System.Windows.Controls.Border
            {
                Width = element.Width,
                Height = element.Height,
                Background = new SolidColorBrush(ToColor(element.BackgroundColor)),
                BorderBrush = new SolidColorBrush(ToColor(element.BorderColor)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(6),
                Child = new System.Windows.Controls.TextBlock
                {
                    Text = element.Text,
                    Foreground = new SolidColorBrush(ToColor(element.ForegroundColor)),
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center,
                    TextAlignment = TextAlignment.Center,
                },
            };
        }

        private FrameworkElement CreateSliderVisual(AppElementDefinition element)
        {
            var denominator = Math.Max(1, element.MaxValue - element.MinValue);
            var normalized = Math.Clamp((double)(element.Value - element.MinValue) / denominator, 0.0, 1.0);

            var track = new System.Windows.Controls.Border
            {
                Width = element.Width,
                Height = element.Height,
                Background = new SolidColorBrush(ToColor(element.BackgroundColor)),
                BorderBrush = new SolidColorBrush(ToColor(element.BorderColor)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(12),
            };

            var fill = new System.Windows.Controls.Border
            {
                Width = Math.Max(18, element.Width * normalized),
                Height = Math.Max(8, element.Height - 10),
                Margin = new Thickness(5),
                Background = new SolidColorBrush(ToColor(element.ForegroundColor)),
                CornerRadius = new CornerRadius(10),
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Center,
            };

            track.Child = fill;
            return track;
        }

        private FrameworkElement CreateTextFieldVisual(AppElementDefinition element)
        {
            return new System.Windows.Controls.Border
            {
                Width = element.Width,
                Height = element.Height,
                Background = new SolidColorBrush(ToColor(element.BackgroundColor)),
                BorderBrush = new SolidColorBrush(ToColor(element.BorderColor)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(6),
                Child = new System.Windows.Controls.TextBlock
                {
                    Text = element.Text,
                    Foreground = new SolidColorBrush(ToColor(element.ForegroundColor)),
                    Margin = new Thickness(10, 7, 10, 7),
                    TextWrapping = TextWrapping.NoWrap,
                },
            };
        }

        private FrameworkElement CreateImageVisual(AppElementDefinition element)
        {
            var container = new System.Windows.Controls.Border
            {
                Width = element.Width,
                Height = element.Height,
                Background = new SolidColorBrush(ToColor(element.BackgroundColor)),
                BorderBrush = new SolidColorBrush(ToColor(element.BorderColor)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(8),
            };

            if (string.IsNullOrWhiteSpace(element.AssetPath) || !File.Exists(element.AssetPath))
            {
                container.Child = new System.Windows.Controls.TextBlock
                {
                    Text = "Missing asset",
                    Foreground = Brushes.White,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center,
                };
                return container;
            }

            container.Child = new System.Windows.Controls.Image
            {
                Stretch = Stretch.UniformToFill,
                Source = new BitmapImage(new Uri(element.AssetPath)),
            };

            return container;
        }

        private static Color ToColor(uint value)
        {
            return Color.FromRgb(
                (byte)((value >> 16) & 0xFF),
                (byte)((value >> 8) & 0xFF),
                (byte)(value & 0xFF));
        }

        private void AddPanel_Click(object sender, RoutedEventArgs e) => AddElement(AppElementType.Panel);

        private void AddLabel_Click(object sender, RoutedEventArgs e) => AddElement(AppElementType.Label);

        private void AddButton_Click(object sender, RoutedEventArgs e) => AddElement(AppElementType.Button);

        private void AddSlider_Click(object sender, RoutedEventArgs e) => AddElement(AppElementType.Slider);

        private void AddTextField_Click(object sender, RoutedEventArgs e) => AddElement(AppElementType.TextField);

        private void AddImage_Click(object sender, RoutedEventArgs e) => AddElement(AppElementType.Image);

        private void AddElement(AppElementType type)
        {
            var root = Document.Elements.First(element => element.Type == AppElementType.RootWindow);
            var parent = SelectedElement != null &&
                         (SelectedElement.Type == AppElementType.Panel || SelectedElement.Type == AppElementType.RootWindow)
                ? SelectedElement
                : root;
            var id = Document.NextElementId();

            var element = new AppElementDefinition
            {
                Id = id,
                ParentId = type == AppElementType.RootWindow ? -1 : parent.Id,
                ZIndex = Document.Elements.Count,
                Type = type,
                Flags = AppElementFlags.Visible | (type == AppElementType.Button || type == AppElementType.Slider || type == AppElementType.TextField ? AppElementFlags.Interactive : AppElementFlags.None),
                X = 24,
                Y = 24 + (id * 6),
                Width = type == AppElementType.Label ? 180 : 160,
                Height = type == AppElementType.Label ? 24 : (type == AppElementType.TextField ? 34 : 42),
                BackgroundColor = type switch
                {
                    AppElementType.Panel => 0x223047,
                    AppElementType.Button => 0x355B88,
                    AppElementType.Slider => 0x0E1520,
                    AppElementType.TextField => 0x101826,
                    AppElementType.Image => 0x223047,
                    _ => 0x1B2333,
                },
                ForegroundColor = type == AppElementType.Slider ? 0x61C48Bu : 0xF4F8FCu,
                BorderColor = 0x8DA0B4,
                Text = type.ToString(),
                MinValue = 0,
                MaxValue = 100,
                Value = 50,
            };

            Document.Elements.Add(element);
            SelectedElement = element;
            StatusMessage = $"Added {type} element #{id}.";
        }

        private void DeleteElement_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedElement == null || SelectedElement.Type == AppElementType.RootWindow)
            {
                return;
            }

            var deletedId = SelectedElement.Id;
            Document.DeleteElement(SelectedElement);
            SelectedElement = Document.Elements.LastOrDefault();
            StatusMessage = $"Deleted element #{deletedId}.";
        }

        private void AddEvent_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedElement == null)
            {
                return;
            }

            var appEvent = new AppEventDefinition { EventType = AppEventType.Click };
            SelectedElement.Events.Add(appEvent);
            SelectedEvent = appEvent;
            StatusMessage = $"Added event to element #{SelectedElement.Id}.";
        }

        private void RemoveEvent_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedElement == null || SelectedEvent == null)
            {
                return;
            }

            SelectedElement.Events.Remove(SelectedEvent);
            SelectedEvent = SelectedElement.Events.FirstOrDefault();
            StatusMessage = $"Removed event from element #{SelectedElement.Id}.";
        }

        private void AddBinding_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedElement == null || SelectedEvent == null)
            {
                return;
            }

            var rootId = Document.Elements.First(element => element.Type == AppElementType.RootWindow).Id;
            var binding = new AppBindingDefinition
            {
                ActionType = AppActionType.SetProperty,
                TargetElementId = rootId,
                PropertyType = AppPropertyType.Text,
            };
            SelectedEvent.Bindings.Add(binding);
            SelectedBinding = binding;
            StatusMessage = $"Added binding to {SelectedEvent.EventType} on element #{SelectedElement.Id}.";
        }

        private void RemoveBinding_Click(object sender, RoutedEventArgs e)
        {
            if (SelectedEvent == null || SelectedBinding == null)
            {
                return;
            }

            SelectedEvent.Bindings.Remove(SelectedBinding);
            SelectedBinding = SelectedEvent.Bindings.FirstOrDefault();
            StatusMessage = "Removed binding.";
        }

        private void Open_Click(object sender, RoutedEventArgs e)
        {
            OpenDocumentFromDialog();
        }

        private void Open_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
        {
            OpenDocumentFromDialog();
        }

        private void OpenDocumentFromDialog()
        {
            try
            {
                var dialog = new OpenFileDialog
                {
                    Filter = "WinOS App Package (*.wdexe)|*.wdexe|All Files (*.*)|*.*",
                    CheckFileExists = true,
                    Title = "Open WinOS .wdexe Package",
                };

                if (dialog.ShowDialog(this) != true)
                {
                    return;
                }

                OpenDocument(dialog.FileName);
            }
            catch (Exception ex)
            {
                StatusMessage = $"Open failed: {ex.Message}";
            }
        }

        public void OpenDocument(string packagePath)
        {
            var solutionRoot = ProjectPaths.FindSolutionRoot(AppContext.BaseDirectory);
            var importedDocument = WdexeImporter.ImportDocument(packagePath, solutionRoot);
            LoadDocument(importedDocument, packagePath);
        }

        private void Export_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var solutionRoot = ProjectPaths.FindSolutionRoot(AppContext.BaseDirectory);
                if (solutionRoot == null)
                {
                    throw new InvalidOperationException("Could not locate the WinOS solution root.");
                }

                var result = WdexeExporter.ExportDocument(Document, solutionRoot, Document.PackageAlias);
                Document.PackageAlias = result.PackageAlias;
                StatusMessage = $"Exported {result.PackageAlias} to {result.PackagePath} with {result.AssetCount} copied asset(s).";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Export failed: {ex.Message}";
            }
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
