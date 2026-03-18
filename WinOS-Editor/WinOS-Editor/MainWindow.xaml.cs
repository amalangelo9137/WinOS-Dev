using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;

namespace WinOS_Editor
{
    // --- ENGINE CORE CLASSES ---
    public class GameObject
    {
        public string Name { get; set; }
        public List<Component> Components { get; set; } = new List<Component>();
        public GameObject(string name) { Name = name; }
        public void AddComponent(Component c) => Components.Add(c);
        public T GetComponent<T>() where T : Component => (T)Components.FirstOrDefault(c => c is T);
    }

    public abstract class Component { public string ComponentName { get; protected set; } }

    public class Transform : Component
    {
        public int X { get; set; } = 50; public int Y { get; set; } = 50;
        public int W { get; set; } = 100; public int H { get; set; } = 100;
        public Transform() { ComponentName = "Transform"; }
    }

    public class UIRenderer : Component
    {
        public string ColorHex { get; set; } = "0x007ACC";
        public UIRenderer() { ComponentName = "UI Renderer"; }
    }

    public class UIImageRenderer : Component
    {
        public Image Image { get; set; }
        public UIImageRenderer() { ComponentName = "Image Renderer"; }
    }

    // --- MAIN WINDOW LOGIC ---
    public partial class MainWindow : Window
    {
        private List<GameObject> SceneObjects = new List<GameObject>();
        private GameObject selectedObject;

        public MainWindow()
        {
            InitializeComponent();
        }

        // 1. HIERARCHY & CONTEXT MENU
        private void CreateSquare_Click(object sender, RoutedEventArgs e)
        {
            var obj = new GameObject("New Square_" + SceneObjects.Count);
            obj.AddComponent(new Transform());
            obj.AddComponent(new UIRenderer());
            SceneObjects.Add(obj);
            RefreshEditor();
        }

        private void CreateEmpty_Click(object sender, RoutedEventArgs e)
        {
            SceneObjects.Add(new GameObject("Empty Object"));
            RefreshEditor();
        }

        private void DeleteObject_Click(object sender, RoutedEventArgs e)
        {
            if (selectedObject != null)
            {
                SceneObjects.Remove(selectedObject);
                selectedObject = null;
                RefreshEditor();
            }
        }

        private void HierarchyTree_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            if (e.NewValue is TreeViewItem tvi)
            {
                selectedObject = SceneObjects.FirstOrDefault(o => o.Name == tvi.Header.ToString());
                DrawInspector(selectedObject);
            }
        }

        private void RefreshEditor()
        {
            HierarchyTree.Items.Clear();
            foreach (var obj in SceneObjects) HierarchyTree.Items.Add(new TreeViewItem { Header = obj.Name, Foreground = Brushes.White });
            UpdateSceneView();
        }

        // 2. SCENE VIEW RENDERING
        private void UpdateSceneView()
        {
            SceneCanvas.Children.Clear();
            foreach (var obj in SceneObjects)
            {
                var t = obj.GetComponent<Transform>();
                var r = obj.GetComponent<UIRenderer>();
                if (t != null)
                {
                    Rectangle rect = new Rectangle
                    {
                        Width = t.W,
                        Height = t.H,
                        Fill = (r != null) ? GetBrush(r.ColorHex) : Brushes.Transparent,
                        Stroke = Brushes.Cyan,
                        StrokeThickness = (obj == selectedObject) ? 2 : 0
                    };
                    Canvas.SetLeft(rect, t.X); Canvas.SetTop(rect, t.Y);
                    SceneCanvas.Children.Add(rect);
                }
            }
        }

        private Brush GetBrush(string hex)
        {
            try { return (Brush)new BrushConverter().ConvertFromString(hex.Replace("0x", "#")); }
            catch { return Brushes.DeepPink; }
        }

        // 3. DYNAMIC INSPECTOR
        private void DrawInspector(GameObject target)
        {
            InspectorPanel.Children.Clear();
            if (target == null) return;

            AddHeader("GameObject", null, target);
            AddField("Name", target.Name, (v) => { target.Name = v; RefreshEditor(); });

            foreach (var comp in target.Components.ToList())
            {
                AddHeader(comp.ComponentName, comp, target);
                if (comp is Transform t)
                {
                    AddField("X", t.X.ToString(), (v) => { t.X = int.Parse(v); UpdateSceneView(); });
                    AddField("Y", t.Y.ToString(), (v) => { t.Y = int.Parse(v); UpdateSceneView(); });
                    AddField("W", t.W.ToString(), (v) => { t.W = int.Parse(v); UpdateSceneView(); });
                    AddField("H", t.H.ToString(), (v) => { t.H = int.Parse(v); UpdateSceneView(); });
                }
                else if (comp is UIRenderer r)
                {
                    AddField("Color Hex", r.ColorHex, (v) => { r.ColorHex = v; UpdateSceneView(); });
                }
            }

            // Add Component Button
            Button addCompBtn = new Button
            {
                Content = "Add Component",
                Margin = new Thickness(0, 20, 0, 0),
                Background = new SolidColorBrush(Color.FromRgb(55, 55, 55)),
                Foreground = Brushes.White
            };

            // Simple Context Menu for adding
            ContextMenu menu = new ContextMenu();
            MenuItem addTransform = new MenuItem { Header = "Transform" };
            addTransform.Click += (s, e) => { target.AddComponent(new Transform()); DrawInspector(target); };
            MenuItem addUI = new MenuItem { Header = "UI Renderer" };
            addUI.Click += (s, e) => { target.AddComponent(new UIRenderer()); DrawInspector(target); };
            MenuItem addUIImage = new MenuItem { Header = "Image Renderer" };
            addUIImage.Click += (s, e) => { target.AddComponent(new UIImageRenderer()); DrawInspector(target); };

            menu.Items.Add(addTransform);
            menu.Items.Add(addUI);
            menu.Items.Add(addUIImage);
            addCompBtn.Click += (s, e) => { menu.IsOpen = true; };

            InspectorPanel.Children.Add(addCompBtn);
        }

        private void AddHeader(string title, Component c, GameObject t)
        {
            DockPanel dp = new DockPanel { Margin = new Thickness(0, 10, 0, 5) };
            if (c != null)
            {
                Button del = new Button { Content = "✕", Width = 20, Background = Brushes.Transparent, Foreground = Brushes.Red, BorderThickness = new Thickness(0) };
                del.Click += (s, e) => { t.Components.Remove(c); DrawInspector(t); UpdateSceneView(); };
                DockPanel.SetDock(del, Dock.Right); dp.Children.Add(del);
            }
            dp.Children.Add(new TextBlock { Text = title.ToUpper(), Foreground = Brushes.Gray, FontWeight = FontWeights.Bold });
            InspectorPanel.Children.Add(dp);
        }

        private void AddField(string label, string value, Action<string> onChange)
        {
            InspectorPanel.Children.Add(new TextBlock { Text = label, Foreground = Brushes.Silver, FontSize = 11 });
            TextBox tb = new TextBox { Text = value, Background = Brushes.DimGray, Foreground = Brushes.White, BorderThickness = new Thickness(0), Margin = new Thickness(0, 0, 0, 8) };
            tb.TextChanged += (s, e) => onChange(tb.Text);
            InspectorPanel.Children.Add(tb);
        }
    }
}