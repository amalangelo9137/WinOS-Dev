using System.Collections.Generic;
using System.Linq;

namespace WinOS_Editor
{
    // The base container
    public class GameObject
    {
        public string Name { get; set; }
        public List<Component> Components { get; set; } = new List<Component>();

        public GameObject(string name)
        {
            Name = name;
        }

        public void AddComponent(Component component)
        {
            Components.Add(component);
        }

        public T GetComponent<T>() where T : Component
        {
            return (T)Components.FirstOrDefault(c => c is T);
        }
    }

    // The base class for all components
    public abstract class Component
    {
        public string ComponentName { get; protected set; }
    }

    // COMPONENT: Transform (Everything needs a position!)
    public class Transform : Component
    {
        public int X { get; set; }
        public int Y { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }

        public Transform(int x, int y, int w, int h)
        {
            ComponentName = "Transform";
            X = x; Y = y; Width = w; Height = h;
        }
    }

    // COMPONENT: UI Renderer (Draws a color or button)
    public class UIRenderer : Component
    {
        public string HexColor { get; set; }
        public string Text { get; set; }

        public UIRenderer(string color, string text = "")
        {
            ComponentName = "UI Renderer";
            HexColor = color;
            Text = text;
        }
    }
}