package com.mak.vrlj.connect;

import java.io.File;
import java.util.*;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
public interface Handler
{
	void someoneSaidAttack();
}

class Initiater {
    private List<Handler> listeners = new ArrayList<Handler>();

    public void addListener(Handler toAdd) {
        listeners.add(toAdd);
    }

    public void sayAttack() {
        System.out.println("Attack!!");

        // Notify everybody that may be interested.
        for (Handler hl : listeners)
            hl.someoneSaidAttack();
    }
}

// Someone interested in "Attack" events
class Responder implements Handler {
    @Override
    public void someoneSaidAttack() {
        System.out.println("Attack there...");
    }
}