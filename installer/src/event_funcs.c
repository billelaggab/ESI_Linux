#include "event_funcs.h"

#define INST_SCRIPT "./scripts/install.sh"

void* install(void* ins){

	installer* inst=(installer*)ins;

	char *autolog=malloc(5);
	if(inst->uinfo.auto_login)
		strcpy(autolog,"yes");
	else
		strcpy(autolog,"no");

	// arguments for the install.sh script
	char* arg[]={INST_SCRIPT,(char*)inst->pinfo.selected_partition,inst->uinfo.username,inst->uinfo.password,inst->uinfo.password,inst->linfo.language,inst->linfo.keyboard,inst->uinfo.hostname,inst->linfo.timezone,autolog,NULL};

	ps_info psinfo=script_ctrl(INST_SCRIPT,arg);

	//open the pipe between the installation_script process and the installer process
	FILE *script_stream = fdopen(psinfo.stdout_fd, "r");
		if (script_stream == NULL){
			perror("can't open file descriptor");
			return NULL;
		}

		char buffer[256];
		char step[2];
		int pos=0;

		while ( fgets(buffer, sizeof(buffer), script_stream) ) {
			
			if(isdigit(*buffer)){
				step[0]=buffer[0];
				step[1]='\0';	
				pos=atoi(step);

				installation_step_done(inst,pos);
				g_print("%s",buffer);
			}else {//open an error dialog
				installation_step_error(inst,pos+1,&buffer[6]);
				g_print("%s",buffer);
				break;
				
			}
		}

		// closing the pipes read ends
		fclose(script_stream);

		int pid_status;
		//waiting for the child process to terminate
		if ( waitpid(psinfo.pid,&pid_status,0) == -1 ){
			perror("ERROR while waiting for child process");
			return NULL;
		}
		//checking the exit status of the child process
		if (WEXITSTATUS(pid_status)){
			perror("ERROR exit status of the child process is diffrent from zero");
			return NULL;
    }
    
    //enable next button
    if(pos==6)
    	gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
}

void installation_step_done(installer* inst,int pos){

	const int max=6;

	if(pos>max)
		return;

	gtk_spinner_stop(inst->spinner[pos-1]);

	gtk_widget_set_visible(GTK_WIDGET(inst->image[pos-1]),TRUE);
	gtk_image_set_from_file(inst->image[pos-1],"img/checked.png");

	if(pos!=max)
		gtk_spinner_start(inst->spinner[pos]);		
	
}

void installation_step_error(installer* inst,int pos,char* error){

	const int max=6;

	strcat(error,"Veuillez réessayer");

	if(pos>max)
		return;

	gtk_spinner_stop(inst->spinner[pos-1]);

	gtk_widget_set_visible(GTK_WIDGET(inst->image[pos-1]),TRUE);
	gtk_image_set_from_file(inst->image[pos-1],"img/checked_error.png");

	GtkLabel* label_error=GTK_LABEL(get_child_by_name(GTK_CONTAINER(inst->layouts[6]),"installation_error"));
	gtk_label_set_text(label_error,error);
	
	gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[2]),TRUE);
	
}

void installation_error(installer* inst,char* error){

	GtkMessageDialog *dialog=GTK_MESSAGE_DIALOG(gtk_message_dialog_new(inst->window,
																		GTK_DIALOG_DESTROY_WITH_PARENT,
																		GTK_MESSAGE_INFO,
																		GTK_BUTTONS_CLOSE,
																		"Erreur : %s\nVeuillez réessayer",error));
	g_signal_connect(G_OBJECT(dialog),"response",G_CALLBACK(exit_finish),inst);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));

	while (gtk_events_pending())
    gtk_main_iteration();

	

}

void showpass_event(GtkWidget* w, gpointer data){
		installer* inst = (installer*) data;
		GtkEntry* entry_pass = GTK_ENTRY(gtk_builder_get_object(inst->builders[4],"ent-password"));
		GtkEntry* entry_pass_confirm = GTK_ENTRY(gtk_builder_get_object(inst->builders[4],"ent-passwordConf"));
		GtkImage* showpass_image = GTK_IMAGE(gtk_builder_get_object(inst->builders[4],"showpass_image"));
		if (inst->uinfo.visible==FALSE){
			gtk_entry_set_visibility(entry_pass,TRUE);
			gtk_entry_set_visibility(entry_pass_confirm,TRUE);
			inst->uinfo.visible=TRUE;
		}
		else{
			gtk_entry_set_visibility(entry_pass,FALSE);
			gtk_entry_set_visibility(entry_pass_confirm,FALSE);
			inst->uinfo.visible=FALSE;
		}
}

void open_gparted(GtkWidget* w,gpointer data){
	system("gparted");
}

void refresh_disk_list(GtkWidget* w , gpointer data){
	installer* inst = (installer*) data;
	g_signal_handlers_disconnect_by_func(inst->pinfo.disk_list, init_partition, inst);
	gtk_combo_box_text_remove_all(inst->pinfo.disk_list);
	gtk_combo_box_text_append_text(inst->pinfo.disk_list,"Selectionnez un disque ...");
	init_disk_list(inst);
	gtk_combo_box_set_active(GTK_COMBO_BOX(inst->pinfo.disk_list),0);
	clear_grid(inst->pinfo.partition_grid,0);
  	gtk_label_set_text(inst->pinfo.disk_size,"");
	gtk_label_set_text(inst->pinfo.disk_name,"");
	g_signal_connect(G_OBJECT(inst->pinfo.disk_list),"changed",G_CALLBACK(init_partition),inst);

	gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),FALSE);

}

int isValidName(char* name){

	for(int i=0;i<strlen(name);i++){
		if(!isalnum(name[i]) && name[i]!='-')
			return 0;
	}
	return 1;
}

void alert_dialog(GtkWindow* window,char* message){
	GtkMessageDialog *dialog=GTK_MESSAGE_DIALOG(gtk_message_dialog_new(window,
																		GTK_DIALOG_DESTROY_WITH_PARENT,
																		GTK_MESSAGE_INFO,
																		GTK_BUTTONS_CLOSE,
																		message));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

GtkWidget* get_child_by_name(GtkContainer* c,char* name){
	
	const gchar* wname=gtk_widget_get_name(GTK_WIDGET(c));
    
    if(!strcmp(name,wname))
        return GTK_WIDGET(c);
      	
    if (GTK_IS_CONTAINER(c)){

        GList* l=gtk_container_get_children(GTK_CONTAINER(c));
        GtkWidget* w;
        char* wname;
            
        while(l!=NULL){
            w = get_child_by_name(l->data,name);
                    
            if(w) return w;
                    
            l=g_list_next(l);

        }
    }

    return NULL;

}

void save_user_info(installer* inst){

	if(inst==NULL) return;

	GtkEntry *username,*hostname,*password;
	GtkSwitch* auto_login;

	//getting child widget
	username=GTK_ENTRY(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_username"));
	hostname=GTK_ENTRY(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_hostname"));
	password=GTK_ENTRY(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_password"));
	auto_login=GTK_SWITCH(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_auto_log"));

	//getting user info
	strcpy(inst->uinfo.username,gtk_entry_get_text(username));
	strcpy(inst->uinfo.hostname,gtk_entry_get_text(hostname));
	strcpy(inst->uinfo.password,gtk_entry_get_text(password));

	if(gtk_switch_get_active(auto_login)==TRUE)
		inst->uinfo.auto_login=1;
	else inst->uinfo.auto_login=0;

}

void save_time_lang(installer* inst){

    if(inst==NULL) return;

    GtkComboBoxText *keyboard,*language,*zone,*region;
    char* timezone = malloc(40);

    //getting child widget
    region=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_region");
	zone=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_zone");
    keyboard=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_keyboard");
    language=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_lang");

    //getting time_zone info
    strcpy(inst->linfo.region,gtk_combo_box_text_get_active_text(region));
	strcpy(inst->linfo.zone,gtk_combo_box_text_get_active_text(zone));
	sprintf(timezone,"%s/%s",inst->linfo.region,inst->linfo.zone);
	strcpy(inst->linfo.timezone,timezone);

	
	//keyboard
	if (!strcmp(gtk_combo_box_text_get_active_text(keyboard),"Français (AZERTY)"))
    	strcpy(inst->linfo.keyboard,"fr");
    else
		strcpy(inst->linfo.keyboard,"en");
		
	//lang	
	if (!strcmp(gtk_combo_box_text_get_active_text(language),"français (France)"))
		strcpy(inst->linfo.language,"fr_FR.UTF-8");
	else
		strcpy(inst->linfo.language,"en_US.UTF-8");

   
}

int check_time_lang(installer* inst){

	if(inst==NULL) return 0;

    GtkComboBoxText *keyboard,*language,*zone,*region;

    //getting child widget
    region=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_region");
	zone=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_zone");
    keyboard=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_keyboard");
    language=(GtkComboBoxText*)get_child_by_name(GTK_CONTAINER(inst->layouts[3]),"langtime_lang");

    if(gtk_combo_box_text_get_active_text(zone)==NULL){
    	alert_dialog(inst->window,"Vous devez choisir une zone");
    	return 0;
    }

    return 1;

}

int check_partition_info(installer* inst){
	inst->pinfo.spartition_size=get_partition_size_at_index(inst,get_active_radio_button(inst));
	if (inst->pinfo.spartition_size < 16106127360)
			return 0;
	return 1;
}

int check_user_info(installer* inst){

	if(inst==NULL) return 0;

	GtkEntry *username,*hostname,*password,*password2;
	char *uname,*hname,*pass,*pass2;
	uname=malloc(sizeof(char)*32);
	hname=malloc(sizeof(char)*21);
	pass=malloc(sizeof(char)*21);
	pass2=malloc(sizeof(char)*21);

	//getting child widget
	username=GTK_ENTRY(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_username"));
	hostname=GTK_ENTRY(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_hostname"));
	password=GTK_ENTRY(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_password"));
	password2=GTK_ENTRY(get_child_by_name(GTK_CONTAINER(inst->layouts[4]),"user_password2"));

	//getting user info
	strcpy(uname,gtk_entry_get_text(username));
	strcpy(hname,gtk_entry_get_text(hostname));
	strcpy(pass,gtk_entry_get_text(password));
	strcpy(pass2,gtk_entry_get_text(password2));

	if(strlen(uname)==0){
		alert_dialog(inst->window,"Vous devez entrer un nom d'utilisateur");
		return 0;
	}

	if(!isalpha(*uname)){
		alert_dialog(inst->window,"Le nom d'utilisateur doit commencer par une lettre");
		return 0;	
	}

	if(!isValidName(uname)){
		alert_dialog(inst->window,"Le nom d'utilisateur ne doit contenir que des lettres et des nombres ou un tiret(-) ");
		return 0;
	}

	if(strlen(hname)==0){
		alert_dialog(inst->window,"Vous devez entrer un nom de machine");
		return 0;
	}

	if(!isalpha(*hname)){
		alert_dialog(inst->window,"Le nom de la machine doit commencer par une lettre");
		return 0;	
	}

	if(!isValidName(hname)){
		alert_dialog(inst->window,"Le nom de la machine ne doit contenir que des lettres et des nombres ou un tiret(-) ");
		return 0;
	}

	if(strlen(pass)==0){
		alert_dialog(inst->window,"Vous devez entrer un mot de passe");
		return 0;
	}

	if(strcmp(pass,pass2)){
		alert_dialog(inst->window,"Les deux mots de passe ne sont pas identiques");
		return 0;
	}

	free(uname);
	free(hname);
	free(pass);
	free(pass2);

	return 1;
}

void exit_finish(GtkWidget *w, gpointer userdata){

	installer* inst=(installer*) userdata;

	GtkCheckButton* reboot=GTK_CHECK_BUTTON(get_child_by_name(GTK_CONTAINER(inst->layouts[7]),"check_reboot"));

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(reboot))==TRUE){
		system("reboot");
		sleep(5);
	}
		

	g_application_quit(G_APPLICATION(inst->app));
}


void layout_next(GtkFixed* main_fixed,GtkWidget* selected_layout,GtkWidget* wanted_layout,GtkListBox* listbox,gint selected, gint position){
	replace_layout(main_fixed,selected_layout,wanted_layout);
	gtk_list_box_unselect_row(listbox,gtk_list_box_get_row_at_index(listbox,selected));
	gtk_list_box_row_set_selectable(gtk_list_box_get_row_at_index(listbox,selected),FALSE);
	gtk_list_box_row_set_selectable(gtk_list_box_get_row_at_index(listbox,position),TRUE);
	gtk_list_box_select_row(listbox,gtk_list_box_get_row_at_index(listbox,position));

}




void next_click(GtkApplication* app,gpointer data){

	if(data==NULL) return;

	installer* inst=(installer*)data;

	int checked=1;


	switch(inst->pos){

	  case 0: // welcome
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);

	  break;

  	  case 1: // license
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),FALSE);
		refresh_disk_list(GTK_WIDGET(inst->buttons[4]),inst);

	  break;

	  case 2: // partition
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
		if((checked=check_partition_info(inst))){
					inst->pinfo.selected_partition=get_partition_path_at_index(inst,get_active_radio_button(inst));
					gchar* message = malloc(128);
					sprintf(message,"ESI Linux va être installe dans la partition %s",inst->pinfo.selected_partition);
					alert_dialog(inst->window,message);
				}
                else
					alert_dialog(inst->window,"Il vous faut au moins 15 GiB d'espace disque");

	  break;

	  case 3: // lang time
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
		if(checked=check_time_lang(inst))
			save_time_lang(inst);
		

	  break;

	  case 4: // user
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
		if(checked=check_user_info(inst)){
			save_user_info(inst);
			gtk_button_set_label(inst->buttons[1],"Installer");
			init_summary(inst);
		}

	  break;

	  case 5: // summary
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
		//g_print("username:%s\nhostname:%s\npassword:%s\nautologin:%d\nkeyobard:%s\nlanguage:%s\ntimezone:%s\npartition:%s\n",
		//		inst->uinfo.username,inst->uinfo.hostname,inst->uinfo.password,inst->uinfo.auto_login,inst->linfo.keyboard,inst->linfo.language,inst->linfo.timezone,(char*)inst->pinfo.selected_partition);

	  break;

	  case 6: // installation
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[2]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),FALSE);
	  break;


	}

	//Goto the next layout
	if(checked){
		layout_next(inst->main_fixed,inst->layouts[inst->pos],inst->layouts[inst->pos+1],inst->listbox,inst->pos,inst->pos+1);
		inst->pos++;
	}

	if(inst->pos==6){
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),FALSE);
		gtk_button_set_label(inst->buttons[1],"Suivant");
		//start new thread
		pthread_t tid;
		pthread_create(&tid,NULL,install,inst);
		
	}

}


void back_click(GtkApplication* app,gpointer data){

	if(data==NULL) return;
	installer* inst=(installer*)data;
	switch(inst->pos){

  	  case 1: // License
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
	  break;

	  case 2:  // Partitionement
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
	  break;

	  case 3: // Lang
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
		//disk_choosed(inst);
	  break;

	  case 4: // user
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
	  break;

	  case 5: // summary
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[0]),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(inst->buttons[1]),TRUE);
		gtk_button_set_label(inst->buttons[1],"Suivant");
	  break;

	  case 6: // installation

	  break;

	}

	layout_next(inst->main_fixed,inst->layouts[inst->pos],inst->layouts[inst->pos-1],inst->listbox,inst->pos,inst->pos-1);
	inst->pos--;

}
