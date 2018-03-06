# Modified from demo found at
# http://stackoverflow.com/questions/5286093/display-listbox-with-columns-using-tkinter


'''
Here the TreeView widget is configured as a multi-column listbox
with adjustable column width and column-header-click sorting.
'''

import Tkinter as tk
import tkFileDialog
import tkFont
import tkMessageBox
import ttk
import os
import sys
import shutil
import xml.etree.ElementTree as etree
import xml.dom.minidom

column_title = ['Event Class', 'Subscriber', 'Instance Start (0-255)', 'Instance End (0-255)']
default_data = ['SF_MESSAGE_EVENT_CLASS_EXAMPLE', 'g_example_queue', '0', '255']
subscriber_data = []

list_struct_names = []
root = tk.Tk()

CONST_XML_DEFAULT_NAME = 'sf_message_cfg.xml'
xml_file = None

updated = False
generated = True

edit_add_window = None
help_window = None

class MultiColumnListbox():
    """use a ttk.TreeView as a multicolumn ListBox"""

    def __init__(self):
        self.tree = None
        self._setup_widgets()
        self._build_tree()

    def _setup_widgets(self):
        s = """Click on header to sort by that column
Click 'Add' to add a subscriber
Click 'Remove' to remove subscriber(s)
Click 'Save' to save the configuration XML file
Click 'Generate' to generate data structures for the messaging framework
        """
        msg = ttk.Label(wraplength="4i", justify="left", anchor="n",
            padding=(10, 2, 10, 6), text=s)
        msg.grid(column=0, columnspan=3, row=0)
        # create a treeview with dual scrollbars
        self.tree = ttk.Treeview(columns=column_title, show="headings")
        vsb = ttk.Scrollbar(orient="vertical",
            command=self.tree.yview)
        hsb = ttk.Scrollbar(orient="horizontal",
            command=self.tree.xview)
        self.tree.configure(yscrollcommand=vsb.set,
            xscrollcommand=hsb.set)
        self.tree.grid(column=0, columnspan=3, row=2, sticky='nsew')
        vsb.grid(column=3, row=2, sticky='ns')
        hsb.grid(column=0, columnspan=3, row=3, sticky='ew')
        self.tree.bind("<Double-1>", self.OnDoubleClick)

        # Add File and Help menus
        menu = tk.Menu(root)
        root.config(menu=menu)
        filemenu = tk.Menu(menu)
        menu.add_cascade(label="File", menu=filemenu)
        filemenu.add_command(label="Save", command=self.OnSaveClick)
        filemenu.add_command(label="Save As", command=self.OnSaveAsClick)
        filemenu.add_command(label="Open...", command=self.OnOpenClick)
        filemenu.add_separator()
        filemenu.add_command(label="Exit", command=on_closing)

        helpmenu = tk.Menu(menu)
        menu.add_cascade(label="Help", menu=helpmenu)
        helpmenu.add_command(label="About...", command=self.OnAboutClick)
        
        # Add Button to add subscriber
        self.addButton = ttk.Button(text=u"Add",command=self.OnAddClick)
        self.addButton.grid(column=0,row=1)
        
        # Add Button to remove subscriber
        self.addButton = ttk.Button(text=u"Remove",command=self.OnRemoveClick)
        self.addButton.grid(column=1,row=1)
        
        # Add Button to generate
        self.addButton = ttk.Button(text=u"Generate",command=self.OnGenerateClick)
        self.addButton.grid(column=2,row=1)

    def OnAboutClick(self):
        # Create a pop up for configuration, only continue when configuration is complete
        about_window = tk.Toplevel()
        about_window.wm_title("About Synergy Message Framework Configurator")

        # Create Label for version info
        s = """
Version 1.1

NOTE: This is a prototype configurator.  The configurator may change in the future
and compatibility with the future configurator is not guaranteed.

Features:
 * Sort by column to group subscribers by event class or queue.
 * Add subscribers using provided button.
   NOTE: The popup must be closed when adding is complete to continue to use the
   main configurator.
 * Remove subscribers using the provided button.
 * Edit subscribers by double clicking the subscriber.
   NOTE: The popup must be closed when adding is complete to continue to use the
   main configurator.
 * Save configurations in an XML file.
 * Generate subscriber list structures using the provided button.
 * Reminders to save/generate will pop up when the configurator is closed if changes
   were made but not saved/generated.
 
Limitations:
 * Configuring priority not yet supported.
 * Only the default name from the e2 studio configurator (p_subscriber_lists) is
   currently supported.
        """
        prompt = tk.StringVar()
        prompt.set(s)
        label = tk.Label(about_window,textvariable=prompt, justify="left")
        label.grid(column=0,row=0,sticky='E')
        
        about_window.focus_set()
        about_window.transient(root)
        about_window.grab_set()
        root.wait_window(about_window)

    def OnAddClick(self):
        # Create a pop up for configuration, only continue when configuration is complete
        edit_add_window = tk.Toplevel()
        edit_add_window.wm_title("Add Subscriber")
        self.gui_data = editsubscriber(edit_add_window, default_data)
        # Add Button to save subscriber
        self.addButton = ttk.Button(edit_add_window, text=u"Save",command=self.OnAddSaveClick)
        self.addButton.grid(column=0,row=len(self.gui_data))
        edit_add_window.focus_set()
        edit_add_window.transient(root)
        edit_add_window.grab_set()
        root.wait_window(edit_add_window)

    def OnAddSaveClick(self):
        # Recover data from pop up
        item = []
        index = 0
        for title in column_title:
            item.append(self.gui_data[index].str.get())
            index += 1
        if not item_exists(item):
            # Insert data into tree
            self.tree.insert('', 'end', values=item)
            # Update global list (non-GUI data)
            create_subscriber_data(self.tree)
            # Resize columns
            self.__init__()
            # Update variables to prompt for save/generate on exit
            global updated
            global generated
            updated = True
            generated = False
        else:
            tkMessageBox.showinfo("Info", "Abort. The event class & queue exist in the current configuration.")

    def OnRemoveClick(self):
        # Remove data from tree
        for item in self.tree.selection():
            self.tree.delete(item)
        # Update global list (non-GUI data)
        create_subscriber_data(self.tree)
        # Resize columns
        self.__init__()
        # Update variables to prompt for save/generate on exit
        global updated
        global generated
        updated = True
        generated = False

    def OnOpenClick(self):
        global xml_file
        xml_file = tkFileDialog.askopenfilename(parent=root,title='Choose a file',initialfile=CONST_XML_DEFAULT_NAME,filetypes=[('Message Configurator XML Files', CONST_XML_DEFAULT_NAME)])
        if (None == xml_file):
            return
        parse_xml_info(xml_file)
        self.__init__()

    def OnDoubleClick(self, event):
        row = self.tree.identify_row(event.y)
        col = self.tree.identify_column(event.y)
        if (row and col):
            item = self.tree.selection()[0]
            values = self.tree.item(item,"values")
            # Create a pop up for configuration, only continue when configuration is complete
            edit_add_window = tk.Toplevel()
            edit_add_window.wm_title("Edit Subscriber")
            self.gui_data = editsubscriber(edit_add_window, values)
            # Add Button to save subscriber
            self.addButton = ttk.Button(edit_add_window, text=u"Save",command=self.OnSaveEditClick)
            self.addButton.grid(column=0,row=len(self.gui_data))
            edit_add_window.focus_set()
            edit_add_window.transient(root)
            edit_add_window.grab_set()
            root.wait_window(edit_add_window)

    def OnSaveEditClick(self):
        new_item = []
        item = self.tree.selection()[0]
        values = self.tree.item(item,"values")
        index = 0
        for title in column_title:
            new_item.append(self.gui_data[index].str.get())
            index += 1

        if not item_exists_except_self(new_item, self.tree.index(item)):
            if instance_range_check(new_item):
                self.tree.item(item, values=new_item)
                create_subscriber_data(self.tree)
                # Update variables to prompt for save/generate on exit
                global updated
                global generated
                updated = True
                generated = False
            else:
                tkMessageBox.showinfo("Info", "Abort. The Instance End number must be larger than the one for Instance Start.")
        else:
                tkMessageBox.showinfo("Info", "Abort. The event class & queue exist in the current configuration.")
        
    def OnSaveClick(self):
        if (None == xml_file):
            self.OnSaveAsClick()
        else:
            save_xml_info(xml_file)
        
    def OnSaveAsClick(self):
        global xml_file
        xml_file = tkFileDialog.asksaveasfilename(parent=root,title='Save configuration file as',initialfile=CONST_XML_DEFAULT_NAME,filetypes=[('Message Configurator XML Files', CONST_XML_DEFAULT_NAME)])
        if (None == xml_file):
            return
        save_xml_info(xml_file)

    def OnGenerateClick(self):
        self.OnSaveClick()
        generate_structures(xml_file)
        global generated
        generated = True
        
    def _build_tree(self):
        for col in column_title:
            self.tree.heading(col, text=col.title(),
                command=lambda c=col: sortby(self.tree, c, 0))
            # adjust the column's width to the header string
            self.tree.column(col,
                width=tkFont.Font().measure(col.title()))
        for item in subscriber_data:
            self.tree.insert('', 'end', values=item)
        self._resize_columns()
            
    def _resize_columns(self):
        for item in subscriber_data:
            # adjust column's width if necessary to fit each value
            for ix, val in enumerate(item):
                col_w = tkFont.Font().measure(val)
                if self.tree.column(column_title[ix],width=None)<col_w:
                    self.tree.column(column_title[ix], width=col_w)

def create_subscriber_data(tree):
    global subscriber_data
    subscriber_data = []
    for child in tree.get_children(''):
        values = tree.item(child,"values")
        subscriber_data.append(values)

def sortby(tree, col, descending):
    """sort tree contents when a column header is clicked on"""
    # grab values to sort
    data = [(tree.set(child, col), child) \
        for child in tree.get_children('')]
    # if the data to be sorted is numeric change to float
    #data =  change_numeric(data)
    # now sort the data in place
    data.sort(reverse=descending)
    for ix, item in enumerate(data):
        tree.move(item[1], '', ix)
    # switch the heading so it will sort in the opposite direction
    tree.heading(col, command=lambda col=col: sortby(tree, col, \
        int(not descending)))

class subscriber_gui(object):
    __slots__ = ['str', 'entry', 'prompt', 'label']

def editsubscriber(window, values):
    column_index = 0
    gui_data = []
    for title in column_title:
        temp = subscriber_gui()
        gui_data.append(temp)
        # Create Entry box for title
        temp.str = tk.StringVar()
        temp.str.set(values[column_index])
        temp.entry = tk.Entry(window,textvariable=temp.str)
        temp.entry.grid(column=1,row=column_index,sticky='E')

        # Create Label for title
        temp.prompt = tk.StringVar()
        temp.prompt.set(u"Enter " + title + ":")
        temp.label = tk.Label(window,textvariable=temp.prompt)
        temp.label.grid(column=0,row=column_index,sticky='E')

        column_index += 1;

    return gui_data

# Parses information from module's XML files and puts in subscriber data list
def parse_xml_info(xml_file):
    CONST_XML_ROOT = 'classes'
    
    # Parse XML file
    try:
        tree = etree.parse(xml_file)
        tree_root = tree.getroot()
    except:
        return

    # Check root tag
    if tree_root.tag != CONST_XML_ROOT:
        # Invalid XML, move on
        print 'Invalid XML root (should be classes)'
        return

    # Iterate over 'class' elements
    global subscriber_data
    subscriber_data = []
    for class_ in tree_root.findall('class'):
        # Get subscribers. Could error check here for valid elements. Not doing this now.
        try:                    
            name_text = class_.find('name').text
            subscribers = class_.find('subscribers')
            # Iterate over 'subscriber' elements
            for subscriber in subscribers.findall('subscriber'):
                # Get subscribers. Could error check here for valid elements. Not doing this now.
                try:                    
                    queue = subscriber.find('queue').text
                    start = subscriber.find('start').text
                    end = subscriber.find('end').text

                    subscriber_data.append((name_text, queue, start, end))
                except:
                    print 'Invalid XML in subscriber tag'
        except:
            print 'Invalid XML in class tag'

def add_queue(subscribers_element, queue, start, end):
    subscriber = etree.SubElement(subscribers_element, 'subscriber')
    queue_element = etree.SubElement(subscriber, 'queue')
    queue_element.text = queue
    start_element = etree.SubElement(subscriber, 'start')
    start_element.text = start
    end_element = etree.SubElement(subscriber, 'end')
    end_element.text = end

def add_class(classes_element, class_, queue, start, end):
    class_element = etree.SubElement(classes_element, 'class')
    name_element = etree.SubElement(class_element, 'name')
    name_element.text = class_
    subscribers_element = etree.SubElement(class_element, 'subscribers')
    add_queue(subscribers_element, queue, start, end)

# Stores XML info to file
def save_xml_info(xml_file):
    CONST_XML_ROOT = 'classes'
    
    # Create XML tree
    tree = etree.Element(CONST_XML_ROOT)

    # Iterate over subscriber data and populate XML
    for element in subscriber_data:
        class_found = False
        queue_found = False
        # Iterate over 'class' elements
        for class_ in tree.findall('class'):
            try:                    
                name_text = class_.find('name').text
                if (name_text == element[0]):
                    class_found = True
                    # Look for the current queue
                    subscribers = class_.find('subscribers')
                    # Iterate over 'subscriber' elements
                    for subscriber in subscribers.findall('subscriber'):
                        # Get subscribers. Could error check here for valid elements. Not doing this now.
                        try:                    
                            queue = subscriber.find('queue').text
                            if (queue == element[1]):
                                queue_found = True
                                break
                            start = subscriber.find('start').text
                            end = subscriber.find('end').text
                        except:
                            print 'Invalid XML in subscriber tag'
                    if not queue_found:
                        # Add queue to subscriber list
                        add_queue(subscribers, element[1], element[2], element[3])
            except:
                print 'Invalid XML in class tag'

        if not class_found:
            add_class(tree, element[0], element[1], element[2], element[3])

    # Save XML to file
    with open(xml_file, 'w') as f:
        f.write(etree.tostring(tree))

    # Convert XML to pretty formatting
    input_xml = xml.dom.minidom.parse(xml_file)
    pretty_xml_as_string = input_xml.toprettyxml()
    with open(xml_file, "w") as f:
        f.write(pretty_xml_as_string)

    global updated
    updated = False

def item_exists(new_item):
    for item in subscriber_data:
        same = True
        for index in range(0, 2):
            if new_item[index] != item[index]:
                same = False
        if same:
            return True
    return False

def item_exists_except_self(new_item, index_new_item):
    for item in subscriber_data:
        if(subscriber_data.index(item) != index_new_item):
            same = True
            for index in range(0, 2):
                if new_item[index] != item[index]:
                    same = False
            if same:
                return True
    return False

def instance_range_check(new_item):
    if new_item[2] > new_item[3]:
        return False
    else:
        return True

def generate_main_list(cfg_file, main_list_name):
    string = """
/** List of all subscriber definitions. */
sf_message_subscriber_list_t * """ + main_list_name + """[] =
{
"""
    cfg_file.write(string)
    for subscriber in list_struct_names:
        string = """    &""" + subscriber + """,
"""
        cfg_file.write(string)
    
    string = """    NULL,
};
"""
    cfg_file.write(string)
    
def generate_subscriber(cfg_file, name, queue, start, end):
    string = """
/** Queue structure must be allocated in another file. */
extern TX_QUEUE """ + queue + """;

/** Subscriber structure for queue """ + name + """ for instance """ + start + """ through """ + end + """. */
static sf_message_subscriber_t """ + name + """ =
{
    .p_queue = &""" + queue + """,
    .instance_range.start = """ + start + """,
    .instance_range.end = """ + end + """,
};
"""
    cfg_file.write(string)

def generate_class_structs(cfg_file, name_text, current_subscribers):
    string = """
/** List of subscribers for """ + name_text + """ event class. */
static sf_message_subscriber_t * gp_group_""" + name_text + """[] =
{
"""
    cfg_file.write(string)
    nodes = 0
    for subscriber in current_subscribers:
        string = """    &""" + subscriber + """,
"""
        cfg_file.write(string)
        nodes += 1
    
    string = """};
"""
    cfg_file.write(string)
    
    string = """
/** Ties """ + name_text + """ event class to list of subscribers for this event class. */
static sf_message_subscriber_list_t g_list_""" + name_text + """ =
{
    .event_class     = (uint16_t) """ + name_text + """,
    .number_of_nodes = """ + str(nodes) + """,
    .pp_subscriber_group = gp_group_""" + name_text + """
};
"""
    cfg_file.write(string)

    list_struct_names.append("g_list_" + name_text)
    

def generate_structures(xml_file):
    CONST_XML_ROOT = 'classes'
    CONST_CFG_NAME = 'sf_message_cfg.c'

    global list_struct_names
    list_struct_names = []

    cfg_file = tkFileDialog.asksaveasfile(parent=root,mode='w',title='Choose a file',initialfile=CONST_CFG_NAME,filetypes=[('C Files', '.c')])
    if (None == cfg_file):
        return

    string = """/** Generated file - DO NOT EDIT. */
#include "sf_message.h"
"""
    cfg_file.write(string)

    subscriber_struct_names = []
    
    # Parse XML file
    tree = etree.parse(xml_file)
    tree_root = tree.getroot()

    # Check root tag
    if tree_root.tag != CONST_XML_ROOT:
        # Invalid XML, move on
        print 'Invalid XML root (should be classes)'
        return

    # Iterate over 'class' elements
    for class_ in tree_root.findall('class'):
        # Get subscribers. Could error check here for valid elements. Not doing this now.
        try:                    
            name_text = class_.find('name').text
            subscribers = class_.find('subscribers')
            current_subscribers = []
            # Iterate over 'subscriber' elements
            for subscriber in subscribers.findall('subscriber'):
                # Get subscribers. Could error check here for valid elements. Not doing this now.
                try:                    
                    queue = subscriber.find('queue').text
                    start = subscriber.find('start').text
                    end = subscriber.find('end').text

                    struct_name = 'g_subscriber_' + queue + '_' + start + '_' + end
                    current_subscribers.append(struct_name)
                    if struct_name not in subscriber_struct_names:
                        subscriber_struct_names.append(struct_name)

                        generate_subscriber(cfg_file, struct_name, queue, start, end)
                    
                except:
                    print 'Invalid XML in subscriber tag'

            generate_class_structs(cfg_file, name_text, current_subscribers)
        except:
            print 'Invalid XML in class tag'

    main_list_name = "p_subscriber_lists"
    generate_main_list(cfg_file, main_list_name)

def on_closing():
    if updated:
        if tkMessageBox.askyesno("Save", "Do you want to save your configuration?"):
            listbox.OnSaveClick()
    if not generated:
        if tkMessageBox.askyesno("Generate Structures", "Do you want to generate configuration structures?"):
            listbox.OnGenerateClick()
    root.destroy()

if __name__ == '__main__':
    root.title("Messaging Framework Configuration")
    root.protocol("WM_DELETE_WINDOW", on_closing)
    if len(sys.argv) > 1:
        xml_file_dir = sys.argv[1]
        parse_xml_info(os.path.join(xml_file_dir, CONST_XML_DEFAULT_NAME))
    if not os.path.exists('sf_message_port.h'):
        if tkMessageBox.askyesno("Generate sf_message_port.h", "Do you want to generate initial sf_message_port.h?"):
            ref_file = os.path.join('..', 'synergy', 'ssp', 'src', 'framework', 'sf_message', 'ref', 'sf_message_port_ref.h')
            if os.path.exists(ref_file):
                shutil.copyfile(ref_file, 'sf_message_port.h')
    listbox = MultiColumnListbox()
    root.mainloop()
