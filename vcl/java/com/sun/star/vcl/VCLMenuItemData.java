/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Edward Peterlin, September 2004
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 Planamesa Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

package com.sun.star.vcl;

import java.awt.Component;
import java.awt.Menu;
import java.awt.MenuBar;
import java.awt.MenuContainer;
import java.awt.MenuComponent;
import java.awt.MenuItem;
import java.awt.MenuShortcut;
import java.awt.CheckboxMenuItem;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.event.ItemListener;
import java.awt.event.ItemEvent;
import java.util.Iterator;
import java.util.LinkedList;

 /**
  * Instances of this class are used to hold information needed to construct a
  * menu item or menu corresponding to VCL menu items. Menus are considered to
  * be lists of these items.
  */
public final class VCLMenuItemData extends Component {

	/**
	 * Delagate object that performs work for us, if applicable.
	 */
	private VCLMenuItemData delegate = null;

	/**
	 * Object that uses this menu item as its delegate. Used to synchronize
	 * data changes back to parent objects.
	 */
	private VCLMenuItemData delegateForObject = null;

	/**
	 * The disposed flag.
	 */
	private boolean disposed = false;

	/**
	 * Unicode string that corresponds to the title.
	 */
	private String title="";

	/**
	 * Keyboard shortcut to use
	 */
	private MenuShortcut keyboardShortcut=null;

	/**
	 * Identifier that is used in Sal events to identify this specific menu
	 * item in VCL events.
	 */
	private short vclID=0;

	/**
	 * Cookie needed to make the association between the C++ VCL Menu object
	 * that is spawning SALEVENT_MENU* events. Used in conjunction with the id.
	 * Note that this is actually a void ** pointer (!).
	 */
	private long vclMenuCookie=0;

	/**
	 * True if this item is a separator, false if not.
	 */
	private boolean isSeparator=false;

	/**
	 * True if this item is a submenu, false if not.
	 */
	private boolean isSubmenu=false;

	/**
	 * If the item is a submenu, list containing all of the menu items
	 * comprising the menu. The items are stored as VCLMenuItemData references.
	 */
	private LinkedList menuItems=new LinkedList();

	/**
	 * If the item has been inserted into menus, this list holds
	 * backreferences to the parent menus. The backreferences are to the
	 * VCLMenuItemData objects for the parent menus.
	 */
	private LinkedList parentMenus=new LinkedList();

	/**
	 * True if this item is enabled, false if not.
	 */
	private boolean isEnabled=true;

	/**
	 * True if this item is a checkbox, false if not.
	 */
	private boolean isCheckbox=false;

	/**
	 * True if this item is checked, false if it is not. If the item is
	 * checked, it should also be a checkbox.
	 */
	private boolean isChecked=false;

	/**
	 * List of AWT objects that have been generated for this set of menu
	 * item data and are being managed by it.
	 */
	private LinkedList awtPeers=new LinkedList();

	/**
	 * Construct and initialize a new <b>VCLMenuItemData</b> instance.
	 *
	 * @param newTitle initial value of the title of the menu item
	 * @param separator <code>true</code> if the item is to be a separator,
	 *  <code>false</code> if it is to be any other type of item
	 * @param id identifier associated with this item that's required for
	 *  posting VCL events
	 * @param cookie cookie used in SALEVENT_MENU* events to tie the id to a
	 *  specific menu
	 */
	public VCLMenuItemData(String newTitle, boolean separator, short id, long cookie) {

		if(separator) {
			isSeparator=true;
			title="-";
		}
		else if(newTitle!=null) {
			title=newTitle;
		}

		vclID=id;
		vclMenuCookie=cookie;

	}

	/**
	 * Clean up as many references to this item and its peers as we can to
	 * allow garbage collection and destruction of their heavyweight peers.
	 * Note that this does not instruct an item's delegate object to be
	 * disposed. Delegates must manually be disposed.
	 */
	public synchronized void dispose() {

		if (disposed)
			return;

		// [ed] 1/8/05 Set the delegate to NULL here so we make sure
		// to leave active the peers for any delegate objects that may
		// be inserted into other menus or menubars.  Bug #308
	
		if(delegate!=null)
			setDelegate(null);
	
		// [ed] 1/9/05 If we have an assigned delegate object, we
		// shouldn't actually remove ourselves from the parent menu
		// or menubar.  If we do, this will cause the arrays for
		// our internal VCLMenuItemData structures to become out
		// of sync with the AWT structures;  removing a delegate must
		// still leave a placeholder in the AWT structure in order to
		// mirror the placeholder's mirror in our internal arrays.
		// The underlying bug is that the parent-for object actually
		// "owns" the peers, not the delegate.
		//
		// Bug 332

		unregisterAllAWTPeers();

		keyboardShortcut=null;
		menuItems=null;
		title=null;
	
		// if we're a delegate for another object, set that object's
		// delegate to null to avoid a dangling reference to a
		// disposed object
	
		if(delegateForObject!=null)
			delegateForObject.setDelegate(null);

		disposed = true;

	}

	/**
	 * Fetch the current title of the menu item.
	 */
	String getTitle() {

		if(delegate!=null)
			return(delegate.getTitle());

		return(title);

	};

	/**
	 * Change the title of the menu item. Any AWT peers will be automatically
	 * updated.
	 *
	 * @param newTitle new title
	 */
	public void setTitle(String newTitle) {

		if(delegate!=null) {
			delegate.setTitle(newTitle);
			return;
		}

		if(!isSeparator) {
			if(newTitle!=null)
				title=newTitle;
			else
				title="";

			// if we're a delegate for an object, set the title for our
			// parent object so if the delegate gets switched the new
			// delegate can retain the title.

			if(delegateForObject!=null)
				delegateForObject.title=title;

			Iterator e=awtPeers.iterator();
			while(e.hasNext()) {
				MenuItem m=(MenuItem)e.next();
				m.setLabel(title);
			}
		}

	}

	/**
	 * Change the keyboard shortcut of the menu item. Any AWT peers will be
	 * automatically updated.
	 *
	 * @param key VCL keycode of the new key to use as the shortcut
	 * @param useShift <code>true</code> if the shift key should additionally
	 *  be required for the shortcut, <code>false</code> if just command is
	 *  needed as modifier
	 */
	public void setKeyboardShortcut(int key, boolean useShift) {

		if(delegate!=null) {
			delegate.setKeyboardShortcut(key, useShift);
			return;
		}

		int newShortcut=VCLEvent.convertVCLKeyCode(key);
		if(newShortcut!=0) {
			keyboardShortcut=new MenuShortcut(newShortcut, useShift);
			Iterator e=awtPeers.iterator();
			while(e.hasNext()) {
				MenuItem m=(MenuItem)e.next();
				m.setShortcut(keyboardShortcut);
			}
		}

	}

	/**
	 * Fetch the current enabled state of the menu item.
	 */
	boolean getEnabled() {

		if(delegate!=null)
			return(delegate.getEnabled());

		return(isEnabled);

	}

	/**
	 * Change the current enabled state of the menu item. Any AWT peers will
	 * be automatically updated.
	 *
	 * @param newEnabled <code>true</code> if the item should be enabled,
	 *  <code>false</code> if it should be disabled
	 */
	public void setEnabled(boolean newEnabled) {

		if(delegate!=null) {
			delegate.setEnabled(newEnabled);
			return;
		}

		isEnabled=newEnabled;

		// if we're the delegate for an object, make sure that the
		// underlying object's state changed as well so if the delegate
		// gets switched the enabled state can get retained

		if(delegateForObject!=null)
			delegateForObject.isEnabled=newEnabled;

		Iterator e=awtPeers.iterator();
		while(e.hasNext()) {
			MenuItem m=(MenuItem)e.next();
			if(isEnabled)
				m.enable();
			else
				m.disable();
		}

	}

	/**
	 * Get the id corresponding to the menu item that should be used to refer
	 * to this menu item in VCL events.
	 */
	short getVCLID() {

		if(delegate!=null)
			return(delegate.getVCLID());

		return(vclID);

	}

	/**
	 * Get the cookie corresponding to the menu item that should be used to
	 * refer to this menu in VCL events.
	 */
	long getVCLCookie() {

		if(delegate!=null)
			return(delegate.getVCLCookie());

		return(vclMenuCookie);

	}

	/**
	 * Fetch the current checked state of the menu item.
	 */
	boolean getChecked() {

		if(delegate!=null)
			return(delegate.getChecked());

		return(isChecked);

	}

	/**
	 * Change the current checked state of the menu item. Note that changing
	 * the checked state may have side effects that invalidate the AWT peers.
	 * Some items, such as separators and items that are actually submenus,
	 * cannot be checked.
	 *
	 * @param newCheck <code>true</code> if the item should be checked,
	 *  <code>false</code> if unchecked
	 * @return <code>true</code> if the state change requires AWT peers to be
	 *  refreshed to appear correctly, <code>false</code> if all required
	 *  changes have been propogated to their peers
	 */
	boolean setChecked(boolean newCheck) {

		if(delegate!=null)
			return(delegate.setChecked(newCheck));

		boolean peersInvalidated=false;

		isChecked=newCheck;
		if(!isCheckbox) {
			if(isChecked) {
				isCheckbox=true;

				// we were just set to checked, so we need to use instances
				// of CheckMenuItem AWT objects instead of regular
				// MenuItems. We need to invalidate our peers.

				unregisterAllAWTPeers();
				peersInvalidated=true;
			}
		}
		else {
			// change state of our checkbox peers

			Iterator e=awtPeers.iterator();
			while(e.hasNext()) {
				MenuItem cMI=(MenuItem)e.next();
				if (cMI instanceof CheckboxMenuItem)
					((CheckboxMenuItem)cMI).setState(isChecked);
			}
		}

		return(peersInvalidated);

	}

	/**
	 * Determine if the item is currently a menu or a regular item. Adding the
	 * first menu item will change an object into a menu
	 *
	 * @return <code>true</code> if object is a menu, <code>false</code> if
	 *  just a single item
	 */
	boolean isMenu() {

		if(delegate!=null)
			return(delegate.isMenu());

		return(isSubmenu);

	}

	/**
	 * Mark a menu item as designated for a submenu prior to the insertion of
	 * any elements into it. This should only be used by VCLMenu constructors.
	 */
	void makeMenu() {

		if(delegate!=null) {
			delegate.makeMenu();
			return;
		}

		isCheckbox=false;
		isSubmenu=true;
		isSeparator=false;
		unregisterAllAWTPeers();

	}

	/**
	 * Add in a new menu item at a particular position in the menu. Note that
	 * adding a menu item may result in invalidating the AWT peers as it
	 * indicates the item must in fact be a menu.
	 *
	 * @param newItem item to be added into the menu
	 * @param nPos position at which the item should be inserted, any item in
	 *  that position or occuring after that position will be pushed down in
	 *  the menu order
	 * @return <code>true</code> if the peers were invalidated and must be
	 *  reinserted into their parents, <code>false</code> if the changes have
	 *  already successfully propogated to any AWT peers
	 */
	boolean addMenuItem(VCLMenuItemData newItem, short nPos) {

		if(delegate!=null)
			return(delegate.addMenuItem(newItem, nPos));

		boolean peersInvalidated=false;

		if(nPos < 0)
			nPos=(short)menuItems.size();

		menuItems.add(nPos, newItem);
		newItem.parentMenus.add(this);
		if(!isSubmenu) {
			isSubmenu=true;
			unregisterAllAWTPeers();
			peersInvalidated=true;
		}
		else {
			Iterator e=awtPeers.iterator();
			while(e.hasNext()) {
				Menu m=(Menu)e.next();
				m.insert((MenuItem)newItem.createAWTPeer(), nPos);
			}
		}

		return(peersInvalidated);

	}

	/**
	 * Remove a menu item at a particular position.  This only applies for
	 * menu style menu items.
	 *
	 * @param nPos position of item to delete
	 */
	void removeMenuItem(short nPos) {

		if(delegate!=null) {
			delegate.removeMenuItem(nPos);
			return;
		}

		if(nPos < 0)
			nPos=(short)menuItems.size();

		VCLMenuItemData i = (VCLMenuItemData)menuItems.get(nPos);
		i.unregisterAllAWTPeers();
		i.parentMenus.remove(this);
		menuItems.remove(nPos);

	}

	/**
	 * Retrieve a menu item at a particular position. This only applies for
	 * menu style menu items.
	 *
	 * @param nPos position of item to retrieve
	 */
	VCLMenuItemData getMenuItem(short nPos) {

		if(delegate!=null)
			return(delegate.getMenuItem(nPos));

		return((VCLMenuItemData)menuItems.get(nPos));

	}

	/**
	 * Determine the position of a specific menu item. This only applies for
	 * menu style menu items.  Comparison is done on a reference level,
	 * <b>not</b>t for items of equivalent contents. The references must match
	 * for the item to be found.
	 *
	 * @param item item whose position should be retrieved
	 * @return index of the item in the menu or -1 if the item is not in the
	 *  menu
	 */
	short getMenuItemIndex(VCLMenuItemData item) {

		if(delegate!=null)
			return(delegate.getMenuItemIndex(item));

		short toReturn=-1;
		short items = (short)menuItems.size();
		for(short i=0; i<items; i++) {
			if(menuItems.get(i)==item) {
				toReturn=i;
				break;
			}
		}

		return(toReturn);

	}

	/**
	 * Fetch the number of menu items in this menu.
	 *
	 * @return number of menu item elements
	 */
	short getNumMenuItems() {

		if(delegate!=null)
			return(delegate.getNumMenuItems());

		return((short)menuItems.size());

	}

	/**
	 * Set the delegate object for this instance.
	 *
	 * @param d	new delegate
	 */
	void setDelegate(VCLMenuItemData d) {

		if (delegate != null)
			delegate.delegateForObject=null;

		delegate=d;

		if (delegate != null)
			delegate.delegateForObject=this;

	}

	/**
	 * Fetch the delegate object for this instance
	 *
	 * @return delegate object reference or null if there is no delegate
	 */
	VCLMenuItemData getDelegate() {

		return(delegate);

	}

	/**
	 * Subclass of AWT MenuItem that allows the item to respond to AWT menu
	 * selections by posting appropriate events into the VCL event queue.
	 *
	 * @see MenuItem
	 * @see VCLEventQueue
	 */
	final class VCLAWTMenuItem extends MenuItem implements ActionListener {

		/**
		 * Menu item data associated with this AWT item.
		 */
		private VCLMenuItemData d;

		/**
		 * Construct a new <code>VCLAWTMenuItem</code> instance.
		 *
		 * @param title initial title of the menu item (may be changed later)
		 * @param data VCLMenuItemData holding the information needed to bind
		 *  the AWT item to a VCL item
		 */
		VCLAWTMenuItem(String title, VCLMenuItemData data) {

			super(title);
			d=data;
			addActionListener(this);

		}

		/**
		 * Respond to menu item choices by posting appropriate events into the
		 * event queue.
		 *
		 * @param e event spawning this action
		 */
		public synchronized void actionPerformed(ActionEvent e) {

			if (disposed)
				return;

			VCLMenuBar mb=VCLMenuBar.findVCLMenuBar(this);
			if (mb!=null) {
				VCLEventQueue q = mb.getEventQueue();
				VCLFrame f = mb.getFrame();
				if (q != null && f != null) {
					q.postCachedEvent(new VCLEvent(VCLEvent.SALEVENT_MENUACTIVATE, f, d.getVCLID(), d.getVCLCookie()));
					q.postCachedEvent(new VCLEvent(VCLEvent.SALEVENT_MENUCOMMAND, f, d.getVCLID(), d.getVCLCookie()));
					q.postCachedEvent(new VCLEvent(VCLEvent.SALEVENT_MENUDEACTIVATE, f, d.getVCLID(), d.getVCLCookie()));
				}
			}

		}

	}

	/**
	 * Subclass of AWT CheckboxMenuItem that allows the item to respond to AWT
	 * menu selections by posting appropriate events into the VCL event queue.
	 *
	 * @see CheckboxMenuItem
	 * @see VCLEventQueue
	 */
	final class VCLAWTCheckboxMenuItem extends CheckboxMenuItem implements ItemListener {

		/**
		 * Menu item data associated with this AWT item.
		 */
		private VCLMenuItemData d;

		/**
		 * Construct a new <code>VCLAWTCheckboxMenuItem</code> instance.
		 *
		 * @param title initial title of the menu item (may be changed later)
		 * @param data VCLMenuItemData holding the information needed to bind
		 *  the AWT item to a VCL item
		 * @param state initial checked state of the menu item
		 */
		VCLAWTCheckboxMenuItem(String title, VCLMenuItemData data, boolean state) {

			super(title);
			d=data;
			addItemListener(this);
			setState(state);

		}

		/**
		 * Respond to menu item choices by posting appropriate events into the
		 * queue.
		 *
		 * @param e	event spawning this action
		 */
		public synchronized void itemStateChanged(ItemEvent e) {

			if (disposed)
				return;

			VCLMenuBar mb=VCLMenuBar.findVCLMenuBar(this);
			if(mb!=null) {
				VCLEventQueue q = mb.getEventQueue();
				VCLFrame f = mb.getFrame();
				if (q != null && f != null) {
					q.postCachedEvent(new VCLEvent(VCLEvent.SALEVENT_MENUACTIVATE, f, d.getVCLID(), d.getVCLCookie()));
					q.postCachedEvent(new VCLEvent(VCLEvent.SALEVENT_MENUCOMMAND, f, d.getVCLID(), d.getVCLCookie()));
					q.postCachedEvent(new VCLEvent(VCLEvent.SALEVENT_MENUDEACTIVATE, f, d.getVCLID(), d.getVCLCookie()));
				}
			}

			// Fix bug 2407 by resetting the state to match the VCL state
			setState(d.getChecked());
		}

	}

	/**
	 * Using all of the current settings, construct an AWT object that is most
	 * appropriate for these settings. While the resulting object is guaranteed
	 * to be a reference to a MenuItem or one of its subclasses, up to three
	 * types may be returned: MenuItem, CheckboxMenuItem, and Menu. The
	 * returned object will be added onto internal queues to be automatically
	 * synchronized with changes made to this menu item.
	 *
	 * @return AWT MenuItem for this object
	 */
	MenuComponent createAWTPeer() {

		if(delegate!=null)
			return(delegate.createAWTPeer());

		MenuComponent toReturn=null;

		if(isCheckbox) {
			VCLAWTCheckboxMenuItem cmi=new VCLAWTCheckboxMenuItem(getTitle(), this, getChecked());
			if(getEnabled())
				cmi.enable();
			else
				cmi.disable();
			if(keyboardShortcut!=null)
				cmi.setShortcut(keyboardShortcut);
			toReturn=cmi;
		}
		else if(isSubmenu) {
			Menu mn=new Menu(getTitle());
			if(getEnabled())
				mn.enable();
			else
				mn.disable();
			Iterator items=menuItems.iterator();
			while(items.hasNext()) {
				VCLMenuItemData i=(VCLMenuItemData)items.next();
				mn.add((MenuItem)i.createAWTPeer());
			}
			toReturn=mn;
		}
		else if(isSeparator) {
			// separator is a menu item with a label of a dash
			MenuItem sep=new MenuItem("-");
			toReturn=sep;
		}
		else {
			VCLAWTMenuItem mi=new VCLAWTMenuItem(getTitle(), this);
			if(getEnabled())
				mi.enable();
			else
				mi.disable();
			if(keyboardShortcut!=null)
				mi.setShortcut(keyboardShortcut);
			toReturn=mi;
		}

		// add the new peer onto our internal tracking lists

		if(toReturn!=null)
			awtPeers.add(toReturn);

		return(toReturn);

	}

	/**
	 * Reinsert new peer objects for the menu item into all of the registered
	 * parent menus for the menu item.
	 */
	void refreshAWTPeersInParentMenus() {

		LinkedList parentMenusClone = (LinkedList)parentMenus.clone();
		Iterator parents=parentMenusClone.iterator();
		while(parents.hasNext()) {
			VCLMenuItemData parent=(VCLMenuItemData)parents.next();
			short menuPos=parent.getMenuItemIndex(this);
			if(menuPos >= 0) {
				parent.removeMenuItem(menuPos);
				parent.addMenuItem(this, menuPos); // creates a new peer
			}
		}

	}

	/**
	 * Reregister a single AWT menu peer from another item to item.  This will
	 * remove the AWT menu peer awtPeer from the specified menu and reregister
	 * it to this item.
	 *
	 * @param srcItem the item to obtain the AWT menu peer from
	 * @param mb the AWT menubar to obtain the AWT menu peer from
	 * @return <code>true</code> if the reregistration was successful otherwise
	 *  <code>false</code>
	 */
	boolean reregisterAWTPeer(VCLMenuItemData srcItem, MenuBar mb) {

		if(delegate!=null)
			return(delegate.reregisterAWTPeer(srcItem, mb));

		if(srcItem.delegate!=null)
			return(reregisterAWTPeer(srcItem.delegate, mb));

		if (isSubmenu && srcItem.isMenu()) {
			// Reassign the source's peers
			LinkedList destPeers = new LinkedList();
			Iterator e=srcItem.awtPeers.iterator();
			while(e.hasNext()) {
				Menu m=(Menu)e.next();
				if (m.getParent() == mb)
					destPeers.add(m);
			}

			// Remove the peers that we are moving from the source and add
			// it to the destination
			e=destPeers.iterator();
			while(e.hasNext()) {
				Menu m=(Menu)e.next();
				awtPeers.add(m);
				srcItem.awtPeers.remove(m);
				if(isEnabled)
					m.enable();
				else
					m.disable();
				m.setLabel(title);

				int oldCount = m.getItemCount();
				Iterator items=menuItems.iterator();
				while(items.hasNext()) {
					VCLMenuItemData i=(VCLMenuItemData)items.next();
					m.add((MenuItem)i.createAWTPeer());
				}

				// Remove preexisting menu items
				while (oldCount-- > 0) 
					m.remove(0);
			}

			return true;
		}
		else {
			return false;
		}

	}

	/**
	 * Unregister all AWT peer objects that have been created from the menu
	 * item data. If the item data corresponds to a submenu, all of the submenu
	 * peers will also be unregistered.
	 */
	void unregisterAllAWTPeers() {

		if(delegate!=null) {
			delegate.unregisterAllAWTPeers();
			return;
		}

		Iterator peers=awtPeers.iterator();
		while(peers.hasNext()) {
			MenuItem mi=(MenuItem)peers.next();
			if(mi instanceof VCLAWTMenuItem)
				mi.removeActionListener((VCLAWTMenuItem)mi);
			else if(mi instanceof VCLAWTCheckboxMenuItem)
				((CheckboxMenuItem)mi).removeItemListener((VCLAWTCheckboxMenuItem)mi);
			mi.setShortcut(null);

			MenuContainer mc = mi.getParent();
			if (mc instanceof Menu) {
				Menu m = (Menu)mc;
				m.remove(mi);
			}
		}

		awtPeers.clear();

	}

}
