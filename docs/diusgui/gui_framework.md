# GUI Framework

> [!WARNING]
> This is an completely WIP.

## What is a GUI?

We have this model:

```txt
[Events] -> [Application State] -> [Widget Tree] -> [Layout] -> [Paint]
```

In this diagram, these phases are shown sequentially. In reality, we want to perform some of these stages independently
of each other.

Example 1: When drawing visual animation, we need to paint at the refresh-rate of the monitor, regardless of what the
application is doing.

Example 2: When resizing a window, we need to relayout at the refresh-rate of the monitor, regardless of what the
application is doing.

Given this model, it is clear that the widget tree must include all necessary information to perform layout and
painting, while also supporting modification

Under a normal "retained" mode GUI, layout is stateful and functions as a property of a widget. A widget provides an
override paint method to perform drawing.

With an "immediate" mode GUI, the application state is transformed into a temporary tree, which has to be reconciled
with the previous tree to support persistent state (i.e. buttons). This is basically the ReactJS model.

Events typically get sent to specific widgets (for a mouse event the widget which was clicked on), usually also with
some sort of bubbling. So if a widget ignores an event it goes to its parent.

## This System

What I want is for the GUI system to behave like React, in that there is a "render" function which produces a tree of
children. These children are only actually created if they have changed since the last iteration.

We can model events as an async sequence sender, which exists for each widget. However, this doesn't handle the
frameworks need to get an event response.

For mouse events, we need to know whether or not the event should be bubbled up. For an event like application quitting
we need to be able to reject this request (to say show a popup saying that a file needs to be save first). In the async
sequence model, communication is only 1 directional. If we wanted to send replies, we would need a second channel.

However, the IPC system supports asynchronous communication. If we model the widget event listener as a "server" the
client can send an event a require a response back. Figuring out a good abstraction for bubbling would probably make
this approach workable.

With an async first model, we can no longer mutate application state directly. We essentially have no control over when
this handler runs so we can race. Because of this, it would make sense to queue changes to the application state. You
could also make the event handler return the new application state directly.
