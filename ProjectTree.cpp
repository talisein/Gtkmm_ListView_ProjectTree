// PROJECT headers
#include "ProjectTree.hh"

// GTKMM headers
#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/label.h>
#include <gtkmm/treeexpander.h>

// STD headers
#include <iostream>
#include <type_traits>
#include <fstream>
#include <utility>

ProjectTree::ProjectTree () :
    Gtk::Box(Gtk::Orientation::VERTICAL)
{
    // Add base widgets
    append(m_tree);

    // ListView styling
    m_tree.set_expand(true);
    m_tree.set_size_request(300, 500);
    m_tree.set_valign(Gtk::Align::FILL);
    m_tree.add_css_class("data-table");

    // Create the tree model
    m_tree_root = create_model();
    m_store = Gtk::TreeListModel::create(m_tree_root, sigc::mem_fun(*this, &ProjectTree::create_model));

    // Creating the selection model
    m_selection_model = Gtk::SingleSelection::create(m_store);
    m_selection_model->set_autoselect(false);
    m_selection_model->set_can_unselect(true);
    m_tree.set_model(m_selection_model);

    // Binding the functions of the tree
    Glib::RefPtr<Gtk::SignalListItemFactory> project_tree_factory =
        Gtk::SignalListItemFactory::create();
    project_tree_factory->signal_bind().connect(
        sigc::mem_fun(*this, &ProjectTree::on_bind_row));
    project_tree_factory->signal_setup().connect(
        sigc::mem_fun(*this, &ProjectTree::on_setup_row));
    m_tree.set_factory(project_tree_factory);

    // signal handlers for buttons
    m_button_action_selection.signal_clicked().connect(
            sigc::mem_fun(*this, &ProjectTree::on_action_selection_button_clicked));

    // Button styling
    m_button_action_selection.set_label("...");
    m_button_action_selection.set_expand(false);
    Gtk::Box* button_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    button_box->append(m_button_action_selection);
    m_button_action_selection.set_valign(Gtk::Align::END);
    append(*button_box);

    // Creating the popover menu
    m_actions_popup_menu.set_parent(m_button_action_selection);
    m_actions_popup_menu.set_has_arrow(true);
    Glib::RefPtr<Gio::Menu> popover_menu = Gio::Menu::create();
    popover_menu->append("_Add Row", "popover.add");
    popover_menu->append("_Remove Row", "popover.remove");
    popover_menu->append("_Unselect", "popover.unselect");
    m_actions_popup_menu.set_menu_model(popover_menu);
    Glib::RefPtr<Gio::SimpleActionGroup> action_group = Gio::SimpleActionGroup::create();
    action_group->add_action("add",
        sigc::bind(sigc::mem_fun(*this, &ProjectTree::AddRow)));
    action_group->add_action("remove",
        sigc::bind(sigc::mem_fun(*this, &ProjectTree::RemoveRow)));
    action_group->add_action("unselect",
        sigc::bind(sigc::mem_fun(*this, &ProjectTree::Unselect)));
    m_button_action_selection.insert_action_group("popover", action_group);
}

ProjectTree::~ProjectTree ()
{
    // needed in order to avoid Gtk-WARNING related with detection of button
    // and popover menu
    m_actions_popup_menu.unparent();
}

ProjectTree::ProjectModel::ProjectModel(Gtk::Box* row_box,
                                        Glib::RefPtr<Gio::ListStore<ProjectModel>> parent_store,
                                        const std::vector<ProjectCell>& childs,
                                        Glib::RefPtr<Gio::ListStore<ProjectModel>> child_store) :
    m_row_box(row_box),
    m_parent_store(parent_store),
    m_childs(childs),
    m_child_store(child_store)
{

}

Gtk::Box*
ProjectTree::ProjectModel::GetRowBox() const
{
    return m_row_box;
}

void
ProjectTree::ProjectModel::SetRowBox(Gtk::Box* new_row_box)
{
    m_row_box = new_row_box;
}

std::vector<ProjectTree::ProjectCell>
ProjectTree::ProjectModel::GetChilds() const
{
    return m_childs;
}

void
ProjectTree::ProjectModel::AppendChild(const ProjectCell& child)
{
    m_childs.emplace_back(child);
}

Glib::RefPtr<Gio::ListStore<ProjectTree::ProjectModel>>
ProjectTree::ProjectModel::GetParentStore() const
{
    return m_parent_store;
}

Glib::RefPtr<Gio::ListStore<ProjectTree::ProjectModel>>
ProjectTree::ProjectModel::GetChildStore() const
{
    return m_child_store;
}

void
ProjectTree::ProjectModel::SetChildStore(Glib::RefPtr<Gio::ListStore<ProjectModel>> child_store)
{
    m_child_store = child_store;
}

ProjectTree::ProjectCell::ProjectCell()
{

}

ProjectTree::ProjectCell::ProjectCell(Gtk::Box* row_box,
                                      std::vector<ProjectCell> childs) :
    m_row_box(row_box),
    m_childs(childs)
{

}


ProjectTree::ProjectCell::~ProjectCell()
{

}

Gtk::Box*
ProjectTree::ProjectCell::GetRowBox() const
{
    return m_row_box;
}

void
ProjectTree::ProjectCell::SetRowBox(Gtk::Box* new_row_box)
{
    m_row_box = new_row_box;
}

std::vector<ProjectTree::ProjectCell>
ProjectTree::ProjectCell::GetChilds() const
{
    return m_childs;
}

Glib::RefPtr<Gio::ListModel>
ProjectTree::create_model(const Glib::RefPtr<Glib::ObjectBase>& item)
{
    // Gets the current object model
    Glib::RefPtr<ProjectModel> col =
        std::dynamic_pointer_cast<ProjectModel>(item);

    // If the object is null then it's root
    if (col == nullptr)
    {
        // If the root store wasn't created yet, create it
        if (m_root_store == nullptr)
        {
            m_root_store = Gio::ListStore<ProjectModel>::create();
        }
        // return root store
        return m_root_store;
    }

    // If it's a leaf, the row without child
    // A store should be created so later be able to add childs
    if (col->GetChilds().empty() == true)
    {
        // If it already has a child store return it
        if (col->GetChildStore() != nullptr)
        {
            return col->GetChildStore();
        }

        // Create a new child store
        Glib::RefPtr<Gio::ListStore<ProjectModel>> new_child_store =
            Gio::ListStore<ProjectModel>::create();
        col->SetChildStore(new_child_store);
        return new_child_store;
    }

    // Get the child store
    Glib::RefPtr<Gio::ListStore<ProjectModel>> result = col->GetChildStore();
    // Remove all the previous rows
    result->remove_all();
    // Add all childs to the child store of the row
    std::vector<ProjectCell> children = col->GetChilds();
    for (ProjectCell& child : children)
    {
        Glib::RefPtr<ProjectModel> new_child =
            ProjectModel::create(child.GetRowBox(),
                                 result);
        result->append(new_child);
    }
    return result;
}

void
ProjectTree::on_setup_row(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
    // Creates a TreeExpander and set has row child
    Gtk::TreeExpander* expander = Gtk::make_managed<Gtk::TreeExpander>();
    expander->set_expand(false);
    list_item->set_child(*expander);
}

void
ProjectTree::on_bind_row(const Glib::RefPtr<Gtk::ListItem>& list_item)
{
    Glib::RefPtr<Gtk::TreeListRow> row =
        std::dynamic_pointer_cast<Gtk::TreeListRow>(list_item->get_item());
    if (row == nullptr)
    {
        return;
    }

    Glib::RefPtr<ProjectModel> col =
        std::dynamic_pointer_cast<ProjectModel>(row->get_item());
    if (col == nullptr)
    {
        return;
    }

    Gtk::TreeExpander* row_expander =
        dynamic_cast<Gtk::TreeExpander*>(list_item->get_child());
    if (row_expander == nullptr)
    {
        return;
    }
    row_expander->set_list_row(row);

    // Set the new box has the TreeExpander child
    auto new_box = col->GetRowBox();
    row_expander->set_child(*new_box);
}

void
ProjectTree::on_action_selection_button_clicked()
{
    // Show the popover menu
    m_actions_popup_menu.popup();
    m_actions_popup_menu.set_sensitive(true);
}

Glib::RefPtr<ProjectTree::ProjectModel>
ProjectTree::ProjectModel::create(Gtk::Box* row_box,
                                  Glib::RefPtr<Gio::ListStore<ProjectModel>> parent_store,
                                  const std::vector<ProjectCell>& childs,
                                  Glib::RefPtr<Gio::ListStore<ProjectModel>> child_store)
{
    return Glib::make_refptr_for_instance<ProjectModel>(
        new ProjectModel(row_box, parent_store, childs, child_store));
}

void
ProjectTree::AddRow()
{
    // Create row name
    static unsigned int row_number = 0;
    const std::string row_name = "Row " + std::to_string(row_number++);

    // Create a box to be displayed in the row
    Gtk::Box* new_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    Gtk::Label* new_label = Gtk::make_managed<Gtk::Label>(row_name);
    new_box->append(*new_label);

    // Get parent row item
    Glib::RefPtr<Glib::ObjectBase> raw_item =
        m_selection_model->get_selected_item();
    Glib::RefPtr<Gtk::TreeListRow> row_item =
        std::dynamic_pointer_cast<Gtk::TreeListRow>(raw_item);

    // If the item is NULL then needs to be added to root
    if (raw_item == nullptr)
    {
        // Create a new object and add it to the root
        Glib::RefPtr<ProjectModel> new_server_row =
            ProjectModel::create(new_box, m_root_store, {}, Gio::ListStore<ProjectModel>::create());
        m_root_store->append(new_server_row);
    }
    else
    {
        // Create a new cell and append to the row childs
        auto row_model = std::dynamic_pointer_cast<ProjectModel>(row_item->get_item());
        ProjectCell new_cell{new_box};
        row_model->AppendChild(new_cell);
    }
}

void
ProjectTree::RemoveRow()
{
    // Get row item
    Glib::RefPtr<Glib::ObjectBase> raw_item =
        m_selection_model->get_selected_item();
    Glib::RefPtr<Gtk::TreeListRow> row_item =
        std::dynamic_pointer_cast<Gtk::TreeListRow>(raw_item);
    g_assert(row_item); // who'd remove a row that doesn't exist?
    Glib::RefPtr<ProjectModel> model = std::dynamic_pointer_cast<ProjectModel>(row_item->get_item());

    // Remove the selected row
    guint row_position = m_selection_model->get_selected();
    model->GetParentStore()->remove(row_position);
}

void
ProjectTree::Unselect()
{
    // FIXME: This should unselect the selected row but it isn't
    m_selection_model->unselect_all();
}
